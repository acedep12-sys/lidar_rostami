# Improvements Analysis — What to Implement Now

## Your Terrain Reality Check

Your game has:
- Open terrain with landscape height variations (hills, bumps)
- Small terrain spikes that only partially cover a crouching soldier
- Trenches (natural depressions)
- Sandbags and man-made cover (but sparse)
- Very few spots that provide FULL body cover
- Real gunfight feel: peek, shoot fast, duck back

This changes the cover system requirements significantly:

### What the current scanner gets WRONG for your terrain:
1. **Rejects partial cover** — a hip-height landscape bump that covers
   the torso but exposes the head is currently classified as "bad cover"
   or skipped entirely. In your game, that IS the cover.
2. **Binary crouch/stand** — real terrain cover is a spectrum. A soldier
   behind a 60cm rock should crouch low, behind a 120cm wall they can
   crouch normally, behind a 180cm wall they stand.
3. **No trench detection** — trenches are BELOW ground level. The scanner
   probes horizontally, not downward. A trench provides cover by being
   lower than the surrounding terrain, not by having walls.
4. **Ceiling rejection kills trenches** — if we add ceiling detection
   (improvement 1.1), we'd reject trenches which have "ceiling" (the
   ground above the trench edge).

### What needs to change:

| Issue | Fix |
|---|---|
| Partial cover rejected | Accept ANY cover height >= 40cm. Score proportionally. |
| Head exposure ignored | Track exposed body percentage. AI knows head is visible. |
| No trench detection | Add downward probe: "am I lower than surrounding terrain?" |
| Binary crouch | Continuous crouch height driven by cover height |
| Too few cover points found | Lower quality threshold. In open terrain, bad cover > no cover. |

---

## Improvement-by-Improvement Decision

### FROM THEIR IMPROVEMENTS.md:

| # | Suggestion | Priority | Implement? | Why |
|---|---|---|---|---|
| 1.1 | Reject enclosed spaces | HIGH | ✅ YES but modified | Need ceiling check but exempt trenches |
| 1.2 | Escape path check | HIGH | ✅ YES | Soldiers stuck in corners is a real bug |
| 1.3 | Score by "behind me too" | MEDIUM | ⬜ SKIP for now | Open terrain has minimal flanking geometry |
| 1.4 | Per-direction lean quality | MEDIUM | ✅ YES | Critical for landscape bumps — lean left/right around rocks |
| 1.5 | Vertical cover scoring | HIGH | ✅ YES, CRITICAL | Your terrain IS partial cover — need continuous height |
| 1.6 | Priority-based chunk scan | MEDIUM | ⬜ SKIP | Not bottleneck at your scale |
| 1.7 | Hierarchical voxel octree | LOW | ⬜ SKIP | Over-engineering |
| 2.1 | Multi-sense fusion | MEDIUM | ✅ YES | Improves memory quality for open terrain (long sightlines) |
| 2.2 | Suppression decay slower in cover | MEDIUM | ✅ YES | Critical for your peek-shoot-duck gameplay |
| 2.3 | Predict enemy movement | HIGH | ✅ YES | Open terrain = enemies visible then hidden behind bumps often |
| 2.4 | Custom raycast vision | LOW | ⬜ SKIP | AIPerception is sufficient |
| 2.5 | Hearing through walls | LOW | ⬜ SKIP | Open terrain — no walls to muffle |
| 3.1 | Motion Warping lean | HIGH | ⬜ DEFER | Needs animation assets you don't have yet |
| 3.2 | GASP animations | HIGH | ⬜ DEFER | Needs retargeting work — art task, not code |
| 3.3 | Smart Object approach vectors | HIGH | ✅ YES | Soldiers walking to front of cover is visible |
| 3.4 | Continuous pathfinding | MEDIUM | ⬜ SKIP | Project setting, not code |
| 3.5 | EQS flanking cover | MEDIUM | ✅ YES | Open terrain flanking is a real tactic |
| 3.6 | Cover failure handling | MEDIUM | ✅ YES | Open terrain = many failed cover queries |
| 3.7 | Utility AI scoring | LOW | ⬜ Already have evaluators | |
| 3.8 | Cooperative suppression | LOW | ⬜ SKIP | Have coordinator already |
| 4.3 | Significance-driven ST pause | HIGH | ✅ Already implemented | In AISignificanceManager |
| 6.1 | Death animation → ragdoll | HIGH | ✅ YES | Need animation montage before ragdoll |
| 6.2 | Hit reaction animations | MEDIUM | ✅ YES | Flinch on damage |
| 6.3 | Muzzle flash + shell | MEDIUM | ✅ YES | Visual feedback |
| 9.1 | Player commands squad | HIGH | ⬜ DEFER | Big feature, after core is working |

---

## What I'll Implement in the Remaining Batches

### Cover Scanner Fixes (for your terrain):
1. ✅ Accept partial cover (40cm+) and score proportionally
2. ✅ Continuous cover height (not binary crouch/stand)
3. ✅ Trench detection (downward probe)
4. ✅ Head exposure tracking (AI knows what's visible)
5. ✅ Escape path check (can the soldier retreat?)
6. ✅ Lean quality per direction

### AI Behavior Fixes (for real gunfight feel):
7. ✅ Suppression decay slower in cover
8. ✅ Multi-sense confidence fusion
9. ✅ Enemy movement prediction on fade
10. ✅ Cover failure → advance/retreat instead of idle
11. ✅ Death animation montage → ragdoll transition
12. ✅ Hit reaction flinch montage

### Quest + HUD (remaining batches):
13. ✅ Full quest system with tag-based auto-discovery
14. ✅ Farsi RTL HUD with NativePaint minimap
