//
// Created by otrush on 7/24/2026.
//

#ifndef SHIFT_GPUTIMINGPANEL_HPP
#define SHIFT_GPUTIMINGPANEL_HPP

#include <cfloat>
#include <functional>
#include <string>
#include <vector>

#include "imgui/imgui.h"
#include "EditorPanel.hpp"

#include "Graphics/RHI/Common/GPUProfiling.hpp"

namespace Shift::Editor {
    //! A panel that spefifically shows GPU graphics queue times
    class GPUTimingPanel : public EditorPanel {
    public:
        using RangeProvider = std::function<const std::vector<GPUTimeRange>&()>;

        explicit GPUTimingPanel(RangeProvider provider)
            : EditorPanel("GPU Timing"), m_provider(std::move(provider)) {}

        void OnImGuiRender() override {
            static const std::vector<GPUTimeRange> s_empty;
            const std::vector<GPUTimeRange>& ranges = m_provider ? m_provider() : s_empty;

            if (ranges.empty()) {
                ImGui::TextDisabled("No GPU timing data");
                return;
            }

            //! Frame budget = sum of the top-level ranges (normally the single "Frame" range).
            //! Every bar is drawn as a share of this so nesting reads as a flame-graph column.
            float frameMs = 0.0f;
            for (const GPUTimeRange& r : ranges) {
                if (r.depth == 0) { frameMs += r.milliseconds; }
            }

            ImGui::Text("Frame GPU: %.3f ms", frameMs);
            ImGui::SameLine();
            ImGui::TextDisabled("(%d ranges)", static_cast<int>(ranges.size()));
            ImGui::Separator();

            constexpr ImGuiTableFlags tableFlags =
                ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_SizingStretchProp;

            if (ImGui::BeginTable("GPUTimeRanges", 3, tableFlags)) {
                ImGui::TableSetupColumn("Zone", ImGuiTableColumnFlags_WidthStretch, 0.45f);
                ImGui::TableSetupColumn("ms", ImGuiTableColumnFlags_WidthFixed, 64.0f);
                ImGui::TableSetupColumn("Share", ImGuiTableColumnFlags_WidthStretch, 0.55f);
                ImGui::TableHeadersRow();

                for (int i = 0; i < static_cast<int>(ranges.size()); ++i) {
                    const GPUTimeRange& r = ranges[i];
                    const ImVec4 color = ResolveColor(r);

                    ImGui::PushID(i);
                    ImGui::TableNextRow();

                    //! Zone: indented by nesting depth, prefixed with its color swatch
                    ImGui::TableSetColumnIndex(0);
                    if (r.depth > 0) {
                        ImGui::Dummy(ImVec2(static_cast<float>(r.depth) * 14.0f, 0.0f));
                        ImGui::SameLine(0.0f, 0.0f);
                    }
                    ImGui::ColorButton("##swatch", color,
                        ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoDragDrop, ImVec2(10.0f, 10.0f));
                    ImGui::SameLine();
                    ImGui::TextUnformatted(r.name.c_str());

                    //! Absolute time
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%.3f", r.milliseconds);

                    //! Share of the whole frame as a colored bar
                    ImGui::TableSetColumnIndex(2);
                    const float frac = (frameMs > 0.0f) ? (r.milliseconds / frameMs) : 0.0f;
                    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, color);
                    ImGui::ProgressBar(frac, ImVec2(-FLT_MIN, 0.0f), "");
                    ImGui::PopStyleColor();

                    ImGui::PopID();
                }
                ImGui::EndTable();
            }
        }

    private:
        RangeProvider m_provider;

        //! Use specified color, if not, get it from hame hash
        static ImVec4 ResolveColor(const GPUTimeRange& r) {
            const DebugLabelColor& c = r.color;
            const bool hasTint = (c.r != 0.0f) || (c.g != 0.0f) || (c.b != 0.0f) || (c.a != 0.0f);
            if (hasTint) {
                return ImVec4(c.r, c.g, c.b, (c.a != 0.0f) ? c.a : 1.0f);
            }
            const float hue = static_cast<float>(std::hash<std::string>{}(r.name) % 360u) / 360.0f;
            ImVec4 out{0.0f, 0.0f, 0.0f, 1.0f};
            ImGui::ColorConvertHSVtoRGB(hue, 0.55f, 0.85f, out.x, out.y, out.z);
            return out;
        }
    };
}

#endif //SHIFT_GPUTIMINGPANEL_HPP
