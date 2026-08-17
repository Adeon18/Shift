#!/usr/bin/env python3
"""Generates the committed glTF test assets in this directory.

Run with a plain CPython 3 (stdlib only, nothing to pip install):

    python Assets/Test/generate_box_gltf.py

Why generated rather than downloaded: Khronos' own Box.gltf carries neither UVs nor
tangents and ships as .gltf + .bin, so it cannot exercise the tangent path at all.
These two cubes are the smallest assets that cover both branches of the loader.

  Box.gltf          1x1x1 cube, TWO primitives with a material each, WITH tangents.
                    Covers: primitive merge, submesh ranges, per-slot texture colorspace,
                    a two-level node hierarchy, and the tangent passthrough branch.
  BoxNoTangent.gltf same geometry, ONE primitive, no TANGENT attribute.
                    Covers: the MikkTSpace unindex -> generate -> re-weld branch.
                    36 corners must weld back down to 24 vertices.

Both embed their buffer as a base64 data URI so each asset is a single committed file.
The texture URIs in Box.gltf name files that deliberately do NOT exist: nothing in the
CPU-only loader opens them, and the test only asserts the URI and colorspace that were
recorded for each material slot.
"""

import base64
import json
import math
import os
import struct

# --- geometry ---------------------------------------------------------------

def cross(a, b):
    return (a[1] * b[2] - a[2] * b[1],
            a[2] * b[0] - a[0] * b[2],
            a[0] * b[1] - a[1] * b[0])

def sub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])

def dot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]

def scale(a, s):
    return (a[0] * s, a[1] * s, a[2] * s)

def add(a, b):
    return (a[0] + b[0], a[1] + b[1], a[2] + b[2])

def normalize(a):
    length = math.sqrt(dot(a, a))
    return scale(a, 1.0 / length) if length > 0.0 else (0.0, 0.0, 0.0)

# Each face: normal, U axis, V axis. cross(U, V) == normal keeps the winding
# counter-clockwise seen from outside, which is what glTF calls front-facing.
FACES = [
    ((1.0, 0.0, 0.0),  (0.0, 0.0, -1.0), (0.0, 1.0, 0.0)),
    ((-1.0, 0.0, 0.0), (0.0, 0.0, 1.0),  (0.0, 1.0, 0.0)),
    ((0.0, 1.0, 0.0),  (1.0, 0.0, 0.0),  (0.0, 0.0, -1.0)),
    ((0.0, -1.0, 0.0), (1.0, 0.0, 0.0),  (0.0, 0.0, 1.0)),
    ((0.0, 0.0, 1.0),  (1.0, 0.0, 0.0),  (0.0, 1.0, 0.0)),
    ((0.0, 0.0, -1.0), (-1.0, 0.0, 0.0), (0.0, 1.0, 0.0)),
]

def build_cube():
    """24 vertices (4 per face, so every face gets its own normal and UV square)."""
    positions, normals, uvs, indices = [], [], [], []
    for normal, u_axis, v_axis in FACES:
        center = scale(normal, 0.5)
        base = len(positions)
        # V grows downward in glTF texture space, so the -V corner is the v=1 row.
        corners = [
            (add(add(center, scale(u_axis, -0.5)), scale(v_axis, -0.5)), (0.0, 1.0)),
            (add(add(center, scale(u_axis, 0.5)),  scale(v_axis, -0.5)), (1.0, 1.0)),
            (add(add(center, scale(u_axis, 0.5)),  scale(v_axis, 0.5)),  (1.0, 0.0)),
            (add(add(center, scale(u_axis, -0.5)), scale(v_axis, 0.5)),  (0.0, 0.0)),
        ]
        for position, uv in corners:
            positions.append(position)
            normals.append(normal)
            uvs.append(uv)
        indices += [base + 0, base + 1, base + 2, base + 0, base + 2, base + 3]
    return positions, normals, uvs, indices

def build_tangents(positions, normals, uvs, indices):
    """Per-triangle tangent accumulation, then Gram-Schmidt against the normal.

    The same tangent MikkTSpace produces for a flat quad, which is the point: the
    passthrough branch and the generated branch have to agree on this cube.
    """
    accum_t = [(0.0, 0.0, 0.0)] * len(positions)
    accum_b = [(0.0, 0.0, 0.0)] * len(positions)

    for tri in range(len(indices) // 3):
        i0, i1, i2 = indices[tri * 3], indices[tri * 3 + 1], indices[tri * 3 + 2]
        e1, e2 = sub(positions[i1], positions[i0]), sub(positions[i2], positions[i0])
        duv1 = (uvs[i1][0] - uvs[i0][0], uvs[i1][1] - uvs[i0][1])
        duv2 = (uvs[i2][0] - uvs[i0][0], uvs[i2][1] - uvs[i0][1])
        det = duv1[0] * duv2[1] - duv2[0] * duv1[1]
        r = 1.0 / det if abs(det) > 1e-12 else 0.0
        tangent = scale(sub(scale(e1, duv2[1]), scale(e2, duv1[1])), r)
        bitangent = scale(sub(scale(e2, duv1[0]), scale(e1, duv2[0])), r)
        for i in (i0, i1, i2):
            accum_t[i] = add(accum_t[i], tangent)
            accum_b[i] = add(accum_b[i], bitangent)

    tangents = []
    for i, normal in enumerate(normals):
        t = normalize(sub(accum_t[i], scale(normal, dot(normal, accum_t[i]))))
        # glTF: bitangent = cross(normal, tangent.xyz) * tangent.w
        w = -1.0 if dot(cross(normal, t), accum_b[i]) < 0.0 else 1.0
        tangents.append((t[0], t[1], t[2], w))
    return tangents

# --- glTF assembly ----------------------------------------------------------

def pack_floats(values):
    return b"".join(struct.pack("<" + "f" * len(v), *v) for v in values)

def pack_indices(values):
    return struct.pack("<" + "I" * len(values), *values)

def data_uri(blob):
    return "data:application/octet-stream;base64," + base64.b64encode(blob).decode("ascii")

def axis_min_max(vectors):
    return ([min(v[i] for v in vectors) for i in range(3)],
            [max(v[i] for v in vectors) for i in range(3)])

# glTF component / bufferView target constants, spelled out so the JSON below reads.
FLOAT, UNSIGNED_INT = 5126, 5125
ARRAY_BUFFER, ELEMENT_ARRAY_BUFFER = 34962, 34963


def write_gltf(path, with_tangents, split_primitives):
    positions, normals, uvs, indices = build_cube()
    tangents = build_tangents(positions, normals, uvs, indices) if with_tangents else None

    blob = b""
    views = []

    def add_view(payload, target):
        nonlocal blob
        assert len(blob) % 4 == 0, "accessor byteOffsets must stay 4-byte aligned"
        views.append({"buffer": 0, "byteOffset": len(blob),
                      "byteLength": len(payload), "target": target})
        blob += payload
        return len(views) - 1

    position_view = add_view(pack_floats(positions), ARRAY_BUFFER)
    normal_view = add_view(pack_floats(normals), ARRAY_BUFFER)
    uv_view = add_view(pack_floats(uvs), ARRAY_BUFFER)
    tangent_view = add_view(pack_floats(tangents), ARRAY_BUFFER) if with_tangents else None
    # Split primitives each get their own index view below, numbered from their own vertex window.
    index_view = None if split_primitives else add_view(pack_indices(indices), ELEMENT_ARRAY_BUFFER)

    accessors = []

    def add_accessor(view, offset, count, component_type, type_name, extra=None):
        accessor = {"bufferView": view, "byteOffset": offset, "componentType": component_type,
                    "count": count, "type": type_name}
        if extra:
            accessor.update(extra)
        accessors.append(accessor)
        return len(accessors) - 1

    # Two primitives means two disjoint vertex windows over the same views, addressed by
    # accessor byteOffset, with each primitive's indices numbered from its own window.
    windows = [(0, 12), (12, 12)] if split_primitives else [(0, 24)]
    primitives = []
    for slot, (first_vertex, vertex_count) in enumerate(windows):
        window_positions = positions[first_vertex:first_vertex + vertex_count]
        mins, maxs = axis_min_max(window_positions)
        attributes = {
            "POSITION": add_accessor(position_view, first_vertex * 12, vertex_count,
                                     FLOAT, "VEC3", {"min": mins, "max": maxs}),
            "NORMAL": add_accessor(normal_view, first_vertex * 12, vertex_count, FLOAT, "VEC3"),
            "TEXCOORD_0": add_accessor(uv_view, first_vertex * 8, vertex_count, FLOAT, "VEC2"),
        }
        if with_tangents:
            attributes["TANGENT"] = add_accessor(tangent_view, first_vertex * 16, vertex_count,
                                                 FLOAT, "VEC4")

        first_index = (first_vertex // 4) * 6
        index_count = (vertex_count // 4) * 6
        local = [i - first_vertex for i in indices[first_index:first_index + index_count]]
        local_view = add_view(pack_indices(local), ELEMENT_ARRAY_BUFFER) if split_primitives else index_view
        primitives.append({
            "attributes": attributes,
            "indices": add_accessor(local_view, 0, index_count, UNSIGNED_INT, "SCALAR"),
            "material": slot,
            "mode": 4,  # triangles
        })

    materials = [{
        "name": "BoxMaterialA",
        "pbrMetallicRoughness": {
            "baseColorFactor": [0.8, 0.3, 0.2, 1.0],
            "metallicFactor": 0.25,
            "roughnessFactor": 0.6,
        },
        "emissiveFactor": [0.1, 0.0, 0.0],
        "alphaMode": "OPAQUE",
    }]
    if split_primitives:
        materials[0]["pbrMetallicRoughness"]["baseColorTexture"] = {"index": 0}
        materials[0]["pbrMetallicRoughness"]["metallicRoughnessTexture"] = {"index": 2}
        materials[0]["normalTexture"] = {"index": 1}
        materials[0]["emissiveTexture"] = {"index": 3}
        materials.append({
            "name": "BoxMaterialB",
            "pbrMetallicRoughness": {
                "baseColorFactor": [0.1, 0.6, 0.9, 0.5],
                "metallicFactor": 1.0,
                "roughnessFactor": 0.15,
            },
            "emissiveFactor": [0.0, 0.0, 0.0],
            "alphaMode": "MASK",
            "alphaCutoff": 0.25,
            "doubleSided": True,
        })

    meshes = [{"name": "Box", "primitives": primitives}]
    if split_primitives:
        # A POINTS mesh AHEAD of the box, which the loader must drop (it only handles triangles).
        # That is what makes ModelData::meshes a COMPACTED array: the box is asset mesh 1 but
        # emitted mesh 0, so a node copying its asset mesh index verbatim would point past the end.
        mins, maxs = axis_min_max(positions[0:4])
        points_accessor = add_accessor(position_view, 0, 4, FLOAT, "VEC3", {"min": mins, "max": maxs})
        meshes.insert(0, {"name": "PointCloud",
                          "primitives": [{"attributes": {"POSITION": points_accessor}, "mode": 0}]})

    gltf = {
        "asset": {"version": "2.0", "generator": "Shift Assets/Test/generate_box_gltf.py"},
        "scene": 0,
        "scenes": [{"nodes": [0]}],
        "meshes": meshes,
        "materials": materials,
        "accessors": accessors,
        "bufferViews": views,
        "buffers": [{"byteLength": len(blob), "uri": data_uri(blob)}],
    }

    if split_primitives:
        # A two-level hierarchy so the loader's parent/TRS flattening has something to flatten.
        gltf["nodes"] = [
            {"name": "Root", "translation": [1.0, 2.0, 3.0], "children": [1]},
            # mesh 1, because mesh 0 is the PointCloud the loader drops
            {"name": "BoxNode", "mesh": 1, "scale": [2.0, 2.0, 2.0],
             "rotation": [0.0, 0.70710678, 0.0, 0.70710678]},
        ]
        gltf["images"] = [
            {"uri": "box_basecolor.png"},
            {"uri": "box_normal.png"},
            {"uri": "box_orm.png"},
            {"uri": "box_emissive.png"},
        ]
        gltf["textures"] = [{"source": i} for i in range(4)]
    else:
        gltf["nodes"] = [{"name": "BoxNode", "mesh": 0}]

    with open(path, "w", encoding="utf-8", newline="\n") as f:
        json.dump(gltf, f, indent=1)
        f.write("\n")
    print("wrote {} ({} bytes of vertex/index data)".format(path, len(blob)))


if __name__ == "__main__":
    here = os.path.dirname(os.path.abspath(__file__))
    write_gltf(os.path.join(here, "Box.gltf"), with_tangents=True, split_primitives=True)
    write_gltf(os.path.join(here, "BoxNoTangent.gltf"), with_tangents=False, split_primitives=False)
