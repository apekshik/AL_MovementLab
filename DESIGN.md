# AL_MovementLab - Design Document

## Overview

Apex Legends-inspired momentum-based movement system and projectile weapon system for UE5. Built on `UCharacterMovementComponent` with first-person camera.

## Current Parameter Values

### Base Movement

| Parameter | Value | Description |
|-----------|-------|-------------|
| `WalkSpeed` | 800 | Base movement speed |
| `SprintSpeed` | 1200 | Speed while holding sprint |
| `CrouchWalkSpeed` | 400 | Speed while crouch walking |
| `GravityScale` | 1.8 | Multiplier on world gravity |
| `JumpZVelocity` | 800 | Initial jump velocity |
| `BrakingDecelerationWalking` | 4000 | Quick direction changes |
| `GroundFriction` | 8 | Ground movement friction |
| `MaxAcceleration` | 4000 | Movement acceleration |

### Momentum System

| Parameter | Value | Description |
|-----------|-------|-------------|
| `MaxMomentum` | 100 | Momentum cap |
| `MinSpeedForMomentum` | 400 | Minimum speed to gain momentum |
| `GroundSpeedForMaxGain` | 800 | Speed for maximum momentum gain rate |
| `MomentumGainAtMaxSpeed` | 30 | Momentum/sec at max speed |
| `IdleDecayRate` | 15 | Momentum/sec lost when slow |
| `AirborneGainMultiplier` | 1.25 | Bonus gain while airborne |
| `SprintMomentumMultiplier` | 1.4 | Bonus gain while sprinting |

### Sliding

| Parameter | Value | Description |
|-----------|-------|-------------|
| `MinSpeedToSlide` | 650 | Minimum speed to initiate slide |
| `SlideEnterImpulse` | 800 | Base impulse strength |
| `GroundSlideImpulseMultiplier` | 0.5 | Ground slide impulse modifier |
| `AirSlideImpulseMultiplier` | 1.2 | Air slide impulse modifier |
| `GroundSlideMomentumCost` | 15 | Momentum required for ground slide |
| `SlideMomentumCost` | 30 | Momentum required for air slide |
| `SlideGroundFriction` | 0.2 | Friction while sliding |
| `SlideBrakingDecel` | 150 | Braking deceleration while sliding |

### Wall Running

| Parameter | Value | Description |
|-----------|-------|-------------|
| `MinSpeedToWallRun` | 400 | Minimum speed to start wall run |
| `WallRunSpeed` | 1100 | Speed maintained on wall |
| `WallRunTraceDistance` | 75 | Detection range for walls |
| `WallRunGravityScale` | 0 | Gravity while wall running (float) |
| `WallRunMinVerticalNormal` | 0.9 | How vertical wall must be (1.0 = perfect) |
| `WallJumpHorizontalStrength` | 400 | Push away from wall on jump |
| `WallJumpVerticalStrength` | 150 | Extra vertical boost on wall jump |
| `WallRunCameraTilt` | 7 | Camera roll in degrees |
| `WallRunCooldown` | 0.2 | Seconds before can re-attach after wall jump |

### Double Jump

| Parameter | Value | Description |
|-----------|-------|-------------|
| `DoubleJumpZVelocity` | 900 | Velocity for second jump |

### Air Strafe

| Parameter | Value | Description |
|-----------|-------|-------------|
| `AirStrafeStrength` | 1800 | Velocity added per second when strafing in air |

### Camera

| Parameter | Value | Description |
|-----------|-------|-------------|
| `StandingCameraHeight` | 64 | Camera Z offset when standing |
| `CrouchingCameraHeight` | 32 | Camera Z offset when crouching |
| `CrouchCameraInterpSpeed` | 8 | Height transition speed |
| `WallRunCameraTiltInterpSpeed` | 10 | Roll transition speed |

### Weapon System

| Parameter | Value | Description |
|-----------|-------|-------------|
| `FireRate` | 600 | Rounds per minute |
| `HipfireSpread` | 2 | Spread in degrees when hipfiring |
| `AimTraceDistance` | 50000 | Max distance for aim trace (500m) |
| `ProjectileSpeed` | 18000 | Bullet travel speed |
| `ProjectileGravityScale` | 0.3 | Bullet drop (1.0 = normal gravity) |
| `Damage` | 18 | Damage per hit |

### Weapon Positioning

| Parameter | Value | Description |
|-----------|-------|-------------|
| `HipfireOffset` | (30, 20, -15) | Weapon position when hipfiring |
| `ADSOffset` | (30, 0, -16) | Weapon position when ADS |
| `ADSInterpSpeed` | 15 | Hipfire/ADS transition speed |
| `StowedOffset` | (20, 20, -40) | Weapon position when stowed |
| `StowedRotation` | (45, -90, 0) | Weapon rotation when stowed |
| `StowInterpSpeed` | 12 | Stow/draw transition speed |

### Weapon Recoil

| Parameter | Value | Description |
|-----------|-------|-------------|
| `RecoilKick` | (-3, 0, 1) | Positional kick per shot (back, right, up) |
| `RecoilRotation` | (-2, 0, 0) | Rotational kick per shot (pitch, yaw, roll) |
| `RecoilRecoverySpeed` | 15 | How fast recoil recovers |

## Movement States

```
┌─────────────┐
│   STANDING  │◄─────────────────────────────────────┐
└──────┬──────┘                                      │
       │ Crouch (low speed/momentum)                 │
       ▼                                             │
┌─────────────┐                                      │
│ CROUCH WALK │──────────────────────────────────────┤
└─────────────┘ Release Crouch                       │
       ▲                                             │
       │ Speed < 400                                 │
       │                                             │
┌──────┴──────┐                                      │
│   SLIDING   │──────────────────────────────────────┤
└──────┬──────┘ Release Crouch                       │
       ▲                                             │
       │ Crouch + Speed ≥ 650 + Momentum ≥ 15/30     │
       │                                             │
┌──────┴──────┐                                      │
│  SPRINTING  │──────────────────────────────────────┤
└─────────────┘ Release Sprint                       │
                                                     │
┌─────────────┐                                      │
│ WALL RUNNING│──────────────────────────────────────┤
└──────┬──────┘ Release W / End of wall / Land       │
       │                                             │
       │ Jump                                        │
       ▼                                             │
┌─────────────┐                                      │
│  AIRBORNE   │──────────────────────────────────────┘
└─────────────┘ Land
```

## Weapon States

```
┌──────────────────┐
│  DRAWN (Hipfire) │◄──────────────────┐
└────────┬─────────┘                   │
         │                             │
         │ Hold Right Mouse            │ Press 1
         ▼                             │
┌──────────────────┐                   │
│       ADS        │                   │
└────────┬─────────┘                   │
         │                             │
         │ Release Right Mouse         │
         ▼                             │
┌──────────────────┐                   │
│  DRAWN (Hipfire) │                   │
└────────┬─────────┘                   │
         │                             │
         │ Press 3                     │
         ▼                             │
┌──────────────────┐                   │
│      STOWED      │───────────────────┘
└──────────────────┘
```

## Input Bindings (Legacy Input System)

| Action | Key | Function |
|--------|-----|----------|
| MoveForward | W/S | Forward/backward movement |
| MoveRight | A/D | Strafe movement |
| Turn | Mouse X | Yaw rotation |
| LookUp | Mouse Y | Pitch rotation |
| Sprint | Left Shift | Hold to sprint |
| Jump | Space | Jump / Double jump / Wall jump |
| Crouch | Left Ctrl | Crouch / Slide |
| Fire | Left Mouse | Shoot (hold for auto) |
| ADS | Right Mouse | Aim down sights (hold) |
| DrawWeapon | 1 | Draw primary weapon |
| StowWeapon | 3 | Stow weapon |

## State Transition Rules

### Sliding
- **Ground Slide**: Grounded + Speed ≥ 650 + Momentum ≥ 15
- **Air Slide**: Airborne + Speed ≥ 650 + Momentum ≥ 30 (impulse applied on landing)
- **Exit**: Speed drops below CrouchWalkSpeed → transitions to crouch walk

### Wall Running
- **Entry**: Airborne + Holding W + Speed ≥ 400 + Near vertical wall + Not on cooldown
- **Exit**: Release W, reach end of wall, land, or wall jump
- **Wall Jump**: Preserves forward velocity + pushes away from wall + normal jump height + bonus

### Double Jump
- **Available**: Airborne + Haven't double jumped yet + Not wall running
- **Reset**: On landing OR after wall jump

### Weapon Firing
- **Hipfire**: Projectile spawns from barrel tip, direction calculated toward crosshair aim point
- **ADS**: Projectile spawns from camera center, straight forward, no spread
- **Stowed**: Cannot fire or ADS

## Debug Display

Enabled via `bDrawMomentumDebug = true`. Shows:

1. **Cyan**: Speed and momentum values
2. **Green**: Ground state, sprint state, movement mode
3. **Color-coded**: Slide/wall run eligibility
   - Green: Air slide ready
   - Yellow: Ground slide ready
   - Orange: No slide available

## Architecture

### Core Classes

**AALCharacter** (`ALCharacter.h/.cpp`)
- First-person camera setup
- Input handling and routing
- Camera height/tilt interpolation
- Weapon spawning and management
- Tracks `bWantsToMoveForward` for wall run detection

**UALCharacterMovementComponent** (`ALCharacterMovementComponent.h/.cpp`)
- Extends UCharacterMovementComponent
- All movement state logic
- Public API: `SetIsSprinting()`, `StartCrouch()`, `StopCrouch()`, `TryWallRun()`, `WallJump()`, `DoubleJump()`
- Private internals: `StartGroundSlide()`, `StartAirSlide()`, `StartWallRun()`, `StopWallRun()`, etc.

**AALWeapon** (`ALWeapon.h/.cpp`)
- Projectile-based weapon actor
- Hipfire/ADS/Stowed states with smooth transitions
- Visual recoil system
- Public API: `Fire()`, `StartFire()`, `StopFire()`, `StartADS()`, `StopADS()`, `Stow()`, `Draw()`

**AALProjectile** (`ALProjectile.h/.cpp`)
- Bullet actor with physics
- Uses UProjectileMovementComponent
- Travel time, gravity drop, hit detection

### Key Implementation Details

1. **Engine crouch disabled** (`bCanCrouch = false`) to prevent capsule resize jitter
2. **Camera-only crouch** - height interpolated via `FMath::FInterpTo`
3. **Wall detection** - Line traces left/right from character center
4. **Air slide deferred** - Impulse stored and applied in `ProcessLanded()`
5. **BeginPlay override** - Forces gravity/jump values to override Blueprint defaults
6. **Weapon attached to camera** - Position/rotation managed by weapon's Tick
7. **Hipfire aim trace** - Line trace from camera finds aim point, projectile direction calculated from muzzle to that point
8. **Visual recoil** - Additive offset/rotation that recovers over time

## Future Considerations

- Capsule collision for slides (to slide under obstacles)
- Enhanced Input System migration
- Wall run on tagged surfaces only
- Mantling / ledge grab
- Bunny hop timing bonus
- Weapon switching / multiple weapons
- Ammo system
- Reload animation
- Muzzle flash / tracers
- Camera recoil (screen shake)
