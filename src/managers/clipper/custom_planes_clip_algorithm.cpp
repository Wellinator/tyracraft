/**
 * @brief Optimized Sutherland-Hodgman clipping for PlayStation 2.
 */

#include "managers/clipper/custom_planes_clip_algorithm.hpp"

CustomPlanesClipAlgorithm::CustomPlanesClipAlgorithm() {}
CustomPlanesClipAlgorithm::~CustomPlanesClipAlgorithm() {}


int CustomPlanesClipAlgorithm::clip(std::vector<PlanesClipVertex>& o_vertices,
                                   PlanesClipVertexPtrs* i_vertices,
                                   const EEClipAlgorithmSettings& settings,
                                   Plane* planes) {
  int inCount = 3;
  int outCount = 0;

  for (int i = 0; i < 3; i++) {
    buffers[0][i].position = *i_vertices[i].position;
    buffers[0][i].st = settings.lerpTexCoords ? *i_vertices[i].st : Vec4(0.0f, 0.0f, 0.0f, 0.0f);
    buffers[0][i].color = settings.lerpColors ? *i_vertices[i].color : Vec4(1.0f, 1.0f, 1.0f, 1.0f);
  }

  int currentBuffer = 0;
  for (int p = 0; p < 6; p++) {
    const Plane& plane = planes[p];
    const PlanesClipVertex* input = buffers[currentBuffer];
    PlanesClipVertex* output = buffers[1 - currentBuffer];
    outCount = 0;

    for (int i = 0; i < inCount; i++) {
      const PlanesClipVertex& cur = input[i];
      const PlanesClipVertex& nxt = input[(i + 1) % inCount];

      const float dCur = plane.distanceTo(cur.position);
      const float dNxt = plane.distanceTo(nxt.position);

      const bool curIn = dCur >= 0.0f;
      const bool nxtIn = dNxt >= 0.0f;

      if (curIn && nxtIn) {
        output[outCount++] = nxt;
      } else if (curIn != nxtIn) {
        float t = dCur / (dCur - dNxt);
        PlanesClipVertex& v = output[outCount++];

        // VU0 LERP: v = cur + (nxt - cur) * t
        asm volatile(
            "lqc2       $vf1, 0x00(%1)      \n\t" // cur.position
            "lqc2       $vf2, 0x00(%2)      \n\t" // nxt.position
            "lqc2       $vf3, 0x00(%3)      \n\t" // cur.st
            "lqc2       $vf4, 0x00(%4)      \n\t" // nxt.st
            "lqc2       $vf5, 0x00(%5)      \n\t" // cur.color
            "lqc2       $vf6, 0x00(%6)      \n\t" // nxt.color
            "mfc1       $8, %7              \n\t" // t
            "qmtc2      $8, $vf20           \n\t" // t in vf20.x
            "vsub.xyzw  $vf10, $vf2, $vf1   \n\t" // nxt.pos - cur.pos
            "vsub.xyzw  $vf11, $vf4, $vf3   \n\t" // nxt.st - cur.st
            "vsub.xyzw  $vf12, $vf6, $vf5   \n\t" // nxt.col - cur.col
            "vmulx.xyzw $vf10, $vf10, $vf20 \n\t" // (nxt.pos - cur.pos) * t
            "vmulx.xyzw $vf11, $vf11, $vf20 \n\t" // (nxt.st - cur.st) * t
            "vmulx.xyzw $vf12, $vf12, $vf20 \n\t" // (nxt.col - cur.col) * t
            "vadd.xyzw  $vf1, $vf1, $vf10   \n\t" // interpolated pos
            "vadd.xyzw  $vf3, $vf3, $vf11   \n\t" // interpolated st
            "vadd.xyzw  $vf5, $vf5, $vf12   \n\t" // interpolated color
            "vmove.w    $vf1, $vf0          \n\t" // v.position.w = 1.0f (using constant vf0)
            "sqc2       $vf1, 0x00(%0)      \n\t" // store interpolated position
            :
            : "r"(&v.position), "r"(&cur.position), "r"(&nxt.position),
              "r"(&cur.st), "r"(&nxt.st), "r"(&cur.color), "r"(&nxt.color), "f"(t)
            : "memory", "$8"
        );

        if (settings.lerpTexCoords) {
           asm volatile("sqc2 $vf3, 0x00(%0)" : : "r"(&v.st) : "memory");
        }
        if (settings.lerpColors) {
           asm volatile("sqc2 $vf5, 0x00(%0)" : : "r"(&v.color) : "memory");
        }

        if (!curIn && nxtIn) {
            output[outCount++] = nxt;
        }
      }
    }

    inCount = outCount;
    currentBuffer = 1 - currentBuffer;
    if (inCount < 3) return 0;
  }

  const PlanesClipVertex* finalBuffer = buffers[currentBuffer];
  for (int k = 1; k + 1 < inCount; k++) {
    o_vertices.push_back(finalBuffer[0]);
    o_vertices.push_back(finalBuffer[k]);
    o_vertices.push_back(finalBuffer[k + 1]);
  }

  return (inCount - 2) * 3;
}

Vec4 CustomPlanesClipAlgorithm::intersectPlane(Vec4& plane_n, Vec4& lineStart, Vec4& lineEnd,
                                              float plane_d, float& t) {
  Vec4 dir = lineEnd - lineStart;
  float denom = plane_n.dot3(dir);
  if (std::abs(denom) < 0.0001f) {
    t = 0.0f;
    return lineStart;
  }
  t = -(plane_n.dot3(lineStart) + plane_d) / denom;
  Vec4 res = lineStart + (dir * t);
  res.w = 1.0f;
  return res;
}

u8 CustomPlanesClipAlgorithm::clipAgainstPlane(Triangle& original, PlanesClipVertex* clipped,
                                              const EEClipAlgorithmSettings& settings, Plane* plane) {
  return 0; // Not used anymore
}