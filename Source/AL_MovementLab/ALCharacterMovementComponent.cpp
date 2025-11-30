// ALCharacterMovementComponent.cpp

#include "ALCharacterMovementComponent.h"
#include "GameFramework/Character.h"
#include "Engine/Engine.h"

UALCharacterMovementComponent::UALCharacterMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	// Momentum defaults
	Momentum = 0.f;
	MaxMomentum = 100.f;
	GroundSpeedForMaxGain = 800.f;
	MomentumGainAtMaxSpeed = 30.f;
	MinSpeedForMomentum = 400.f;
	IdleDecayRate = 15.f;
	AirborneGainMultiplier = 1.25f;
	bDrawMomentumDebug = true;

	// Sprint defaults
	WalkSpeed = 600.f;
	SprintSpeed = 900.f;
	SprintMomentumMultiplier = 1.4f;
	bIsSprinting = false;

	// Crouch walk defaults
	CrouchWalkSpeed = 300.f;
	bIsCrouchWalking = false;

	// Slide defaults
	bIsSliding = false;
	bPendingSlideOnLand = false;
	PendingSlideImpulse = 0.f;
	PendingSlideDirection = FVector::ZeroVector;

	MinSpeedToSlide = 650.f;
	SlideEnterImpulse = 800.f;
	GroundSlideImpulseMultiplier = 0.5f;  // Ground slide gets 50% impulse
	AirSlideImpulseMultiplier = 1.2f;     // Air slide gets 120% impulse
	SlideGroundFriction = 0.2f;
	SlideBrakingDecel = 150.f;
	SlideMomentumCost = 30.f;             // Full cost for air slide
	GroundSlideMomentumCost = 15.f;       // Less momentum needed for ground slide

	SavedGroundFriction = 0.f;
	SavedBrakingDecel = 0.f;

	// Set initial walking speed
	MaxWalkSpeed = WalkSpeed;

	// Disable engine crouch
	NavAgentProps.bCanCrouch = false;
}

void UALCharacterMovementComponent::TickComponent(
	float DeltaTime,
	enum ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction
)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const float Speed2D = Velocity.Size2D();

	// ---- MOMENTUM ----

	if (Speed2D > MinSpeedForMomentum && !bIsCrouchWalking)
	{
		float SpeedAlpha = FMath::Clamp(Speed2D / GroundSpeedForMaxGain, 0.f, 1.f);
		float GainPerSecond = FMath::Lerp(0.f, MomentumGainAtMaxSpeed, SpeedAlpha);

		if (!IsMovingOnGround())
		{
			GainPerSecond *= AirborneGainMultiplier;
		}

		if (bIsSprinting)
		{
			GainPerSecond *= SprintMomentumMultiplier;
		}

		Momentum = FMath::Clamp(Momentum + GainPerSecond * DeltaTime, 0.f, MaxMomentum);
	}
	else
	{
		float Decay = IdleDecayRate;

		if (!IsMovingOnGround())
		{
			Decay *= 0.3f;
		}

		if (bIsCrouchWalking)
		{
			Decay *= 2.f;
		}

		Momentum = FMath::Clamp(Momentum - Decay * DeltaTime, 0.f, MaxMomentum);
	}

	// ---- SLIDE TO CROUCH WALK TRANSITION ----
	if (bIsSliding && IsMovingOnGround() && Speed2D < CrouchWalkSpeed)
	{
		StopSlide();
		StartCrouchWalk();
	}

	// ---- DEBUG DISPLAY ----

#if WITH_EDITOR
	if (bDrawMomentumDebug && GEngine && GetOwner())
	{
		const float NormalizedMomentum = (MaxMomentum > 0.f)
			? Momentum / MaxMomentum
			: 0.f;

		const FString GroundState = IsMovingOnGround() ? TEXT("Grounded") : TEXT("Airborne");
		const FString SprintState = bIsSprinting ? TEXT("Sprinting") : TEXT("Walking");

		FString MoveState;
		if (bIsSliding)
		{
			if (bPendingSlideOnLand)
			{
				MoveState = FString::Printf(TEXT("Air Slide (PENDING: %.1f)"), PendingSlideImpulse);
			}
			else
			{
				MoveState = TEXT("Ground Slide");
			}
		}
		else if (bIsCrouchWalking)
		{
			MoveState = TEXT("Crouch Walking");
		}
		else
		{
			MoveState = TEXT("Standing");
		}

		// Line 1: Speed and Momentum
		const FString DebugStr1 = FString::Printf(
			TEXT("Speed2D: %.1f  |  Momentum: %.1f / %.1f (%.0f%%)"),
			Speed2D,
			Momentum,
			MaxMomentum,
			NormalizedMomentum * 100.f
		);

		// Line 2: Movement States
		const FString DebugStr2 = FString::Printf(
			TEXT("%s  |  %s  |  %s"),
			*GroundState,
			*SprintState,
			*MoveState
		);

		// Line 3: Slide Requirements
		const bool bCanSlideSpeed = Speed2D >= MinSpeedToSlide;
		const bool bCanGroundSlide = Momentum >= GroundSlideMomentumCost;
		const bool bCanAirSlide = Momentum >= SlideMomentumCost;
		const bool bIsAirborne = !IsMovingOnGround();

		FString SlideEligibility;
		if (bIsAirborne && bCanSlideSpeed && bCanAirSlide)
		{
			SlideEligibility = TEXT("AIR SLIDE READY");
		}
		else if (!bIsAirborne && bCanSlideSpeed && bCanGroundSlide)
		{
			SlideEligibility = TEXT("GROUND SLIDE READY");
		}
		else
		{
			SlideEligibility = TEXT("NO SLIDE");
		}

		const FString DebugStr3 = FString::Printf(
			TEXT("Speed %s (%.0f/%.0f)  |  Momentum: Ground %s (%.0f/%.0f) Air %s (%.0f/%.0f)  |  %s"),
			bCanSlideSpeed ? TEXT("OK") : TEXT("LOW"),
			Speed2D,
			MinSpeedToSlide,
			bCanGroundSlide ? TEXT("OK") : TEXT("LOW"),
			Momentum,
			GroundSlideMomentumCost,
			bCanAirSlide ? TEXT("OK") : TEXT("LOW"),
			Momentum,
			SlideMomentumCost,
			*SlideEligibility
		);

		GEngine->AddOnScreenDebugMessage(
			(uint64)((PTRINT)this),
			0.f,
			FColor::Cyan,
			DebugStr1
		);

		GEngine->AddOnScreenDebugMessage(
			(uint64)((PTRINT)this) + 1,
			0.f,
			FColor::Green,
			DebugStr2
		);

		FColor SlideColor = FColor::Orange;
		if (bIsAirborne && bCanSlideSpeed && bCanAirSlide)
		{
			SlideColor = FColor::Green;
		}
		else if (!bIsAirborne && bCanSlideSpeed && bCanGroundSlide)
		{
			SlideColor = FColor::Yellow;
		}

		GEngine->AddOnScreenDebugMessage(
			(uint64)((PTRINT)this) + 2,
			0.f,
			SlideColor,
			DebugStr3
		);
	}
#endif
}

// ---- Sprint ----

void UALCharacterMovementComponent::SetIsSprinting(bool bNewSprinting)
{
	if (bIsSprinting == bNewSprinting || bIsSliding || bIsCrouchWalking)
	{
		return;
	}

	bIsSprinting = bNewSprinting;
	MaxWalkSpeed = bIsSprinting ? SprintSpeed : WalkSpeed;
}

// ---- Public Crouch Entry Point ----

void UALCharacterMovementComponent::StartCrouch()
{
	if (bIsSliding || bIsCrouchWalking)
	{
		return;
	}

	const bool bIsAirborne = !IsMovingOnGround();
	const float Speed2D = Velocity.Size2D();
	const bool bHasEnoughSpeed = Speed2D >= MinSpeedToSlide;

	// AIR SLIDE: Airborne with enough speed and full momentum
	if (bIsAirborne && bHasEnoughSpeed && Momentum >= SlideMomentumCost)
	{
		StartAirSlide();
	}
	// GROUND SLIDE: Grounded with enough speed and some momentum
	else if (!bIsAirborne && bHasEnoughSpeed && Momentum >= GroundSlideMomentumCost)
	{
		StartGroundSlide();
	}
	// CROUCH WALK: Not enough speed or momentum for any slide
	else
	{
		StartCrouchWalk();
	}
}

void UALCharacterMovementComponent::StopCrouch()
{
	if (bIsSliding)
	{
		StopSlide();
	}

	if (bIsCrouchWalking)
	{
		StopCrouchWalk();
	}
}

// ---- Crouch Walk (Internal) ----

void UALCharacterMovementComponent::StartCrouchWalk()
{
	if (bIsCrouchWalking)
	{
		return;
	}

	bIsCrouchWalking = true;
	bIsSprinting = false;

	MaxWalkSpeed = CrouchWalkSpeed;
}

void UALCharacterMovementComponent::StopCrouchWalk()
{
	if (!bIsCrouchWalking)
	{
		return;
	}

	bIsCrouchWalking = false;
	MaxWalkSpeed = WalkSpeed;
}

// ---- Ground Slide (Internal) ----

void UALCharacterMovementComponent::StartGroundSlide()
{
	if (bIsSliding)
	{
		return;
	}

	bIsSliding = true;
	bIsSprinting = false;

	// Consume less momentum for ground slide
	Momentum = FMath::Clamp(Momentum - GroundSlideMomentumCost, 0.f, MaxMomentum);

	// Save current movement params
	SavedGroundFriction = GroundFriction;
	SavedBrakingDecel = BrakingDecelerationWalking;

	// Apply slide settings
	GroundFriction = SlideGroundFriction;
	BrakingDecelerationWalking = SlideBrakingDecel;

	// Calculate slide direction
	FVector SlideDir = Velocity;
	SlideDir.Z = 0.f;

	if (!SlideDir.IsNearlyZero())
	{
		SlideDir.Normalize();

		const float MomentumAlpha = (MaxMomentum > 0.f) ? (Momentum / MaxMomentum) : 0.f;

		// Ground slide gets reduced impulse
		float ImpulseStrength = SlideEnterImpulse * MomentumAlpha * GroundSlideImpulseMultiplier;

		if (ImpulseStrength > KINDA_SMALL_NUMBER)
		{
			AddImpulse(SlideDir * ImpulseStrength, true);
		}
	}
}

// ---- Air Slide (Internal) ----

void UALCharacterMovementComponent::StartAirSlide()
{
	if (bIsSliding)
	{
		return;
	}

	bIsSliding = true;
	bIsSprinting = false;

	// Consume full momentum for air slide
	Momentum = FMath::Clamp(Momentum - SlideMomentumCost, 0.f, MaxMomentum);

	// Save current movement params
	SavedGroundFriction = GroundFriction;
	SavedBrakingDecel = BrakingDecelerationWalking;

	// Apply slide settings
	GroundFriction = SlideGroundFriction;
	BrakingDecelerationWalking = SlideBrakingDecel;

	// Calculate slide direction
	FVector SlideDir = Velocity;
	SlideDir.Z = 0.f;

	if (!SlideDir.IsNearlyZero())
	{
		SlideDir.Normalize();

		const float MomentumAlpha = (MaxMomentum > 0.f) ? (Momentum / MaxMomentum) : 0.f;

		// Air slide gets full impulse with airborne multiplier
		float ImpulseStrength = SlideEnterImpulse * MomentumAlpha * AirSlideImpulseMultiplier;

		// Defer impulse until landing
		bPendingSlideOnLand = true;
		PendingSlideImpulse = ImpulseStrength;
		PendingSlideDirection = SlideDir;
	}
}

void UALCharacterMovementComponent::StopSlide()
{
	if (!bIsSliding)
	{
		return;
	}

	bIsSliding = false;

	bPendingSlideOnLand = false;
	PendingSlideImpulse = 0.f;
	PendingSlideDirection = FVector::ZeroVector;

	GroundFriction = SavedGroundFriction;
	BrakingDecelerationWalking = SavedBrakingDecel;
}

void UALCharacterMovementComponent::ProcessLanded(const FHitResult& Hit, float remainingTime, int32 Iterations)
{
	Super::ProcessLanded(Hit, remainingTime, Iterations);

	if (bPendingSlideOnLand && bIsSliding)
	{
		if (PendingSlideImpulse > KINDA_SMALL_NUMBER && !PendingSlideDirection.IsNearlyZero())
		{
			AddImpulse(PendingSlideDirection * PendingSlideImpulse, true);
		}

		bPendingSlideOnLand = false;
		PendingSlideImpulse = 0.f;
		PendingSlideDirection = FVector::ZeroVector;
	}
}