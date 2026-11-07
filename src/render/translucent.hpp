#pragma once

// Two-pass transparency helper (Phase E3).
//
// Pass 1 draws every solid object with normal depth writes. Anything translucent is
// queued here instead, and flush() draws the queue in pass 2: sorted far -> near
// from the camera, with depth *writes* off (depth testing stays on) so a glow
// shell or a particle can never hide the solid object behind or inside it.

#include "raylib.h"
#include "rlgl.h"

#include <algorithm>
#include <functional>
#include <utility>
#include <vector>

namespace snake {
namespace render {

class TranslucentQueue {
public:
    TranslucentQueue() { items_.reserve(256); }

    void add(const Vector3& pos, std::function<void()> draw) {
        items_.push_back(Item{pos, 0.0f, std::move(draw)});
    }

    // Call inside BeginMode3D, after all solid geometry.
    void flush(const Vector3& cameraPos) {
        if (items_.empty()) {
            return;
        }
        for (Item& it : items_) {
            const float dx = it.pos.x - cameraPos.x;
            const float dy = it.pos.y - cameraPos.y;
            const float dz = it.pos.z - cameraPos.z;
            it.distSq = dx * dx + dy * dy + dz * dz;
        }
        std::stable_sort(items_.begin(), items_.end(),
                         [](const Item& a, const Item& b) { return a.distSq > b.distSq; });
        // rlgl batches vertices: flush the solid batch before the mask changes, and the
        // translucent batch before it changes back (raylib 5.5 does not do it for us).
        rlDrawRenderBatchActive();
        rlDisableDepthMask();
        for (Item& it : items_) {
            it.draw();
        }
        rlDrawRenderBatchActive();
        rlEnableDepthMask();
        items_.clear();
    }

private:
    struct Item {
        Vector3 pos;
        float distSq;
        std::function<void()> draw;
    };
    std::vector<Item> items_;
};

} // namespace render
} // namespace snake
