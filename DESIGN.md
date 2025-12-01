# AL_MovementLab - Movement System Design Document

## Overview

Apex Legends-inspired momentum-based movement system for UE5. Built on `UCharacterMovementComponent` with first-person camera.

## Current Parameter Values

### Base Movement

| Parameter | Value | Description |
|-----------|-------|-------------|
| `WalkSpeed` | 800 | Base movement speed |
| `SprintSpeed` | 1200 | Speed while holding sprint |
| `CrouchWalkSpeed` | 400 | Speed while crouch walking |
| `GravityScale` | 1.8 | Multiplier on world gravity |
| `JumpZVelocity` | 800 | Initial jump velocity |

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

### Camera

| Parameter | Value | Description |
|-----------|-------|-------------|
| `StandingCameraHeight` | 64 | Camera Z offset when standing |
| `CrouchingCameraHeight` | 32 | Camera Z offset when crouching |
| `CrouchCameraInterpSpeed` | 8 | Height transition speed |
| `WallRunCameraTiltInterpSpeed` | 10 | Roll transition speed |

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
- Tracks `bWantsToMoveForward` for wall run detection

**UALCharacterMovementComponent** (`ALCharacterMovementComponent.h/.cpp`)
- Extends UCharacterMovementComponent
- All movement state logic
- Public API: `SetIsSprinting()`, `StartCrouch()`, `StopCrouch()`, `TryWallRun()`, `WallJump()`, `DoubleJump()`
- Private internals: `StartGroundSlide()`, `StartAirSlide()`, `StartWallRun()`, `StopWallRun()`, etc.

### Key Implementation Details

1. **Engine crouch disabled** (`bCanCrouch = false`) to prevent capsule resize jitter
2. **Camera-only crouch** - height interpolated via `FMath::FInterpTo`
3. **Wall detection** - Line traces left/right from character center
4. **Air slide deferred** - Impulse stored and applied in `ProcessLanded()`
5. **BeginPlay override** - Forces gravity/jump values to override Blueprint defaults

## Future Considerations

- Capsule collision for slides (to slide under obstacles)
- Enhanced Input System migration
- Wall run on tagged surfaces only
- Mantling / ledge grab
- Bunny hop timing bonus
