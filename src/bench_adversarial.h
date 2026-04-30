#pragma once

// Synthesises four pathological wards in-memory (no JSON), runs five
// strategy combinations on each, prints the per-axis winners.
//
//   all-critical            priority key collapses, can't differentiate
//   long-chain              parallelism = 1, exposes per-decision overhead
//   single-role-bottleneck  starvation: one worker absorbs all the work
//   tight-shift             total work just barely fits — slack vs OT
void run_adversarial_bench();
