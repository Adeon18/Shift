//
// Created by otrush on 10/14/2026.
//

#include "ScenePicker.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

#include "Graphics/Managers/MeshManager.hpp"
#include "Scene/ShiftScene.hpp"

namespace Shift::Graphics {

    namespace {
        void ExpandWorldAABB(const glm::mat4& transform, const Bounds& bounds, glm::vec3& lo, glm::vec3& hi) {
            for (uint32_t i = 0; i < 8; ++i) {
                const glm::vec3 corner{
                    (i & 1u) ? bounds.max.x : bounds.min.x,
                    (i & 2u) ? bounds.max.y : bounds.min.y,
                    (i & 4u) ? bounds.max.z : bounds.min.z
                };
                const glm::vec3 world = glm::vec3(transform * glm::vec4(corner, 1.0f));
                lo = glm::min(lo, world);
                hi = glm::max(hi, world);
            }
        }

        WorldAABB WorldBoxOf(const glm::mat4& transform, const Bounds& bounds) {
            glm::vec3 lo{std::numeric_limits<float>::max()};
            glm::vec3 hi{std::numeric_limits<float>::lowest()};
            ExpandWorldAABB(transform, bounds, lo, hi);
            return {lo, hi};
        }

        void AccumulateSubtree(const ShiftScene& scene, const MeshManager& meshes, NodeID node, glm::vec3& lo, glm::vec3& hi, bool& anyMesh) {
            if (const MeshProperty* meshProperty = scene.GetProperty<MeshProperty>(node)) {
                if (const Mesh* mesh = meshes.Get(meshProperty->mesh)) {
                    ExpandWorldAABB(scene.GetWorld(node), mesh->bounds, lo, hi);
                    anyMesh = true;
                }
            }
            for (const NodeID child : scene.GetChildren(node)) {
                AccumulateSubtree(scene, meshes, child, lo, hi, anyMesh);
            }
        }

        //! Regular ray box intersection
        //! TODO [REFACTOR]: Move this to some separate file
        bool RayHitsAABB(const Ray& ray, const WorldAABB& box, float& tNear, float& tFar) {
            float tMin = 0.0f;
            float tMax = std::numeric_limits<float>::max();

            for (uint32_t axis = 0; axis < 3; ++axis) {
                if (std::abs(ray.direction[axis]) < 1e-8f) {
                    const bool outside = ray.origin[axis] < box.min[axis] || ray.origin[axis] > box.max[axis];
                    if (outside) { return false; }
                    continue;
                }
                const float inverse = 1.0f / ray.direction[axis];
                float t0 = (box.min[axis] - ray.origin[axis]) * inverse;
                float t1 = (box.max[axis] - ray.origin[axis]) * inverse;
                if (t0 > t1) { std::swap(t0, t1); }
                tMin = glm::max(tMin, t0);
                tMax = glm::min(tMax, t1);
                if (tMax < tMin) { return false; }
            }

            tNear = tMin;
            tFar = tMax;
            return true;
        }

        bool ContainsPoint(const WorldAABB& box, const glm::vec3& point) {
            return glm::all(glm::greaterThanEqual(point, box.min)) && glm::all(glm::lessThanEqual(point, box.max));
        }

        //! This is here to avoid picking the aabb if we are inside it so one big object does not ruin our piicking
        struct PickSpan {
            NodeID node = NULL_NODE;
            float tNear = 0.0f;
            float tFar = 0.0f;
        };

        struct PickHits {
            std::vector<PickSpan> entered;
            NodeID enclosing = NULL_NODE;
        };

        void TestSubtree(const ShiftScene& scene, const MeshManager& meshes, const Ray& ray, NodeID node, PickHits& hits) {
            if (const MeshProperty* meshProperty = scene.GetProperty<MeshProperty>(node)) {
                if (const Mesh* mesh = meshes.Get(meshProperty->mesh)) {
                    const glm::mat4 world = scene.GetWorld(node);
                    for (const MeshSubmesh& submesh : mesh->submeshes) {
                        const WorldAABB box = WorldBoxOf(world, submesh.bounds);

                        if (ContainsPoint(box, ray.origin)) {
                            if (hits.enclosing == NULL_NODE) { hits.enclosing = node; }
                            continue;
                        }

                        float tNear = 0.0f;
                        float tFar = 0.0f;
                        if (!RayHitsAABB(ray, box, tNear, tFar)) { continue; }
                        hits.entered.push_back({node, tNear, tFar});
                    }
                }
            }

            for (const NodeID child : scene.GetChildren(node)) {
                TestSubtree(scene, meshes, ray, child, hits);
            }
        }
    }

    std::optional<WorldAABB> SubtreeWorldAABB(const ShiftScene& scene, const MeshManager& meshes, NodeID node) {
        if (!scene.IsValid(node)) { return std::nullopt; }

        glm::vec3 lo{std::numeric_limits<float>::max()};
        glm::vec3 hi{std::numeric_limits<float>::lowest()};
        bool anyMesh = false;
        AccumulateSubtree(scene, meshes, node, lo, hi, anyMesh);

        if (!anyMesh) { return std::nullopt; }
        return WorldAABB{lo, hi};
    }

    Ray MakeViewportRay(const glm::mat4& viewProj, float ndcX, float ndcY) {
        const glm::mat4 inverseViewProj = glm::inverse(viewProj);

        glm::vec4 nearPoint = inverseViewProj * glm::vec4(ndcX, ndcY, 0.0f, 1.0f);
        glm::vec4 farPoint = inverseViewProj * glm::vec4(ndcX, ndcY, 1.0f, 1.0f);
        if (nearPoint.w == 0.0f || farPoint.w == 0.0f) { return {}; }
        nearPoint /= nearPoint.w;
        farPoint /= farPoint.w;

        return {glm::vec3(nearPoint), glm::normalize(glm::vec3(farPoint - nearPoint))};
    }

    NodeID SelectableAncestor(const ShiftScene& scene, NodeID node) {
        NodeID selectable = node;

        for (NodeID n = node; n != NULL_NODE; n = scene.GetParent(n)) {
            const ModelLockProperty* lock = scene.GetProperty<ModelLockProperty>(n);
            if (lock && lock->locked) { selectable = n; }
        }

        return selectable;
    }

    NodeID PickNode(const ShiftScene& scene, const MeshManager& meshes, const Ray& ray) {
        //! Check if raydir is not 1
        if (glm::length(ray.direction) < 0.5f) { return NULL_NODE; }

        PickHits hits{};
        for (const NodeID root : scene.GetRoots()) {
            TestSubtree(scene, meshes, ray, root, hits);
        }

        //! Some straight bullshiii
        NodeID nearest = NULL_NODE;
        float nearestT = std::numeric_limits<float>::max();
        NodeID nearestContainer = NULL_NODE;
        float nearestContainerT = std::numeric_limits<float>::max();

        for (const PickSpan& span : hits.entered) {
            bool contains = false;
            for (const PickSpan& other : hits.entered) {
                if (other.node == span.node) { continue; }
                const bool nested = other.tNear > span.tNear && other.tFar < span.tFar;
                if (nested) {
                    contains = true;
                    break;
                }
            }

            if (contains) {
                if (span.tNear < nearestContainerT) {
                    nearestContainerT = span.tNear;
                    nearestContainer = span.node;
                }
            } else if (span.tNear < nearestT) {
                nearestT = span.tNear;
                nearest = span.node;
            }
        }

        NodeID hit = nearest;
        if (hit == NULL_NODE) { hit = nearestContainer; }
        if (hit == NULL_NODE) { hit = hits.enclosing; }

        return SelectableAncestor(scene, hit);
    }
}
