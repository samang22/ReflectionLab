# Expanding ring attack

## Editor smoke test (after rebuilding)

1. Create a Data Asset of class `RLRingAttackDataAsset`, named `DA_RingAttack`.
2. From Place Actors, place the native `RLExpandingRingAttack` actor on the arena floor. Its origin is the ring center at floor height, not at character capsule height.
3. Assign `Attack Data`. Leave `Auto Start` enabled. Without an asset, built-in defaults apply.
4. Start a round. The placed actor waits through the countdown, expands once, then destroys itself. Placement is optional: enemies also emit rings through the integrated firing rule below.
5. Check standing in the center, walking into the band, parrying the band, and rolling through it. Only the band damages the player; parry does not stop it; rolling rejects damage through the existing player damage gateway.
6. Verify that a successful hit happens at most once per attack, that a roll ending inside the band can still take damage, and that the actor disappears on round clear/game over.

Defaults: start radius 100 cm, expansion speed 450 cm/s, full band width 40 cm, height 70 cm, damage 1, active duration 10 s, fade-out duration 0.5 s. There is no maximum-radius cap: the ring expands for its entire active duration (approximately 4600 cm radius at 10 s with defaults), then freezes its size during fade-out. Fade-out immediately disables damage and gradually reduces FadeOpacity and emissive intensity before destruction. The visual uses hostile-projectile red with emissive intensity 20; its master material enables Instanced Static Mesh usage.

The visual component disables Nanite because the projectile material is translucent. An engine lifespan timer backs up Tick-driven deletion (10.5 s from activation by default, or 0.5 s from an early fade request). These clocks use gameplay time: pause freezes them, and global time dilation slows them. Repeated enemy attacks can create new rings while older ones expire; distinguish individual actors when checking lifetime.

Spawn integration: use Blueprint `Spawn Actor From Class` with `RLExpandingRingAttack` (or a derived Blueprint), a floor-level location, and the shooter as Instigator. Assign the exposed `Attack Data` and `Auto Start` pins before spawning. For manual activation, disable Auto Start and call `Start Attack` during PlayingRound.

## Enemy pattern integration

Normal enemy firing emits this attack every twelve pattern slots (one third of the former frequency). Each enemy starts at a randomized slot, so its first ring may happen on shot 1 through 12; subsequent rings remain twelve shots apart. Tutorial rounds and tutorial-controlled shots are excluded. All shots keep their existing ordered projectile rules, so ring intervals cannot suppress special projectile types. A failed ring spawn does not prevent normal projectile selection.

Each Difficulty asset's `Waves` entry has `Ring Attack`: `Shot Interval` (default 12, 0 disables) and optional `Attack Data`. All 21 waves in the three production difficulty assets explicitly use interval 12. Assets with no authored value inherit the native default. The legacy phase path also supports the rule and disables it in breather phases. Wave breather sections can disable it explicitly with interval 0.

The center is the shooter's floor position at firing time. The attack remains after the shooter dies, but begins fading when the round leaves PlayingRound. Tutorial dodge success also fades the ring immediately. Enemy pool deactivation resets the cached rule; each new wave applies its own rule.

The initial segmented ring is an instanced-mesh functional visualization; warning effects, final art and balance tuning are separate tasks.

## Tutorial integration

Order: move and aim -> roll practice -> normal/perfect/close parry -> explosive -> guard shot / combo -> expanding ring -> defeat the enemy -> reward selection. Movement practice requires both WASD input and cursor movement after Continue. Roll practice requires a successfully started roll to finish, not just pressing Space. Ring practice requires contact with the band during a roll; expiry or walking away does not count and another ring is emitted for retry. Successful practice removes the ring.

SPACE rolls along currently held WASD input using the same camera-relative axes as walking. Diagonal inputs are normalized. With no effective input (including opposing keys cancelling out), the roll follows the character's current forward direction. Mouse aiming remains unchanged for ordinary movement and parrying, but is not used to select the roll direction.

All success transitions use the controller's common `QueueTutorialStage` two-second gameplay-time delay. Repeated events during the delay cannot reset it or skip stages. Existing two-second delay after the guard explanation is preserved, and combo completion requires new parries rather than accepting the previous stages' accumulated count. Prompt pauses do not count toward gameplay-time delays. The ring lesson clears leftover projectiles before firing. Reward selection is locked until the final reward explanation is dismissed, so clicking a card during the delay cannot bypass it.

## Enemy timing variation

`Waves -> Attack Variation` controls pattern-start randomization, initial delay jitter (default additional 0–1 s), burst start interval jitter (default ±20%), and shot gap jitter within a burst (default ±10%). Timers are one-shot and sample new delays each cycle. A burst must finish before another begins, with a minimum 0.1 s pause if it outlasts its sampled interval. These variations are bypassed in tutorial rounds. The shared pattern offset rotates the combined projectile/ring cycle and preserves rule priority and long-term frequency. Offsets are resampled on pool activation and wave/phase assignment; ordinary timer restarts do not reset them. Different enemies may randomly draw the same offset; timing is staggered statistically, not guaranteed unique.

To restore fixed behavior, disable Randomize Pattern Start and set all three jitter values to zero.

## Verification

The feature-specific automation test files have been removed. Verify in gameplay: safe hole and band contact, roll immunity, ten-second continuous expansion followed by a half-second fade, randomized enemy pattern offsets and timings, and the tutorial's common two-second transition delay. Build and gameplay checks are performed by the developer.
