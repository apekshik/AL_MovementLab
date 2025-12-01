// ALCharacterMovementComponent.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ALCharacterMovementComponent.generated.h"

UCLASS()
class AL_MOVEMENTLAB_API UALCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	UALCharacterMovementComponent();

	virtual void BeginPlay() override;

	virtual void TickComponent(
		float DeltaTime,
		enum ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction
	) override;

	virtual void ProcessLanded(const FHitResult& Hit, float remainingTime, int32 Iterations) override;

	void SetIsSprinting(bool bNewSprinting);
	void StartCrouch();
	void StopCrouch();
	bool IsSliding() const { return bIsSliding; }
	bool IsCrouchWalking() const { return bIsCrouchWalking; }

	// ---- Wall Run Public API ----
	void TryWallRun();
	void WallJump();
	bool IsWallRunning() const { return bIsWallRunning; }
	bool IsWallRunningOnRightSide() const { return bWallRunIsRightSide; }
	FVector GetWallRunNormal() const { return WallRunNormal; }
	float GetWallRunCameraTilt() const { return WallRunCameraTilt; }

	// ---- Double Jump Public API ----
	bool CanDoubleJump() const;
	void DoubleJump();

protected:
	// ---- Momentum ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Momentum")
	float Momentum;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Momentum")
	float MaxMomentum;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Momentum")
	float GroundSpeedForMaxGain;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Momentum")
	float MomentumGainAtMaxSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Momentum")
	float MinSpeedForMomentum;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Momentum")
	float IdleDecayRate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Momentum")
	float AirborneGainMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Momentum|Debug")
	bool bDrawMomentumDebug;

	// ---- Sprint ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Sprint")
	float WalkSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Sprint")
	float SprintSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Sprint")
	float SprintMomentumMultiplier;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|Sprint")
	bool bIsSprinting;

	// ---- Crouch Walk ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Crouch")
	float CrouchWalkSpeed;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|Crouch")
	bool bIsCrouchWalking;

	// ---- Slide ----
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|Slide")
	bool bIsSliding;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|Slide")
	bool bPendingSlideOnLand;

	/** Minimum speed to trigger any slide */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Slide")
	float MinSpeedToSlide;

	/** Base impulse for slide entry */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Slide")
	float SlideEnterImpulse;

	/** Multiplier for ground slide impulse (less than airborne) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Slide")
	float GroundSlideImpulseMultiplier;

	/** Multiplier for airborne slide impulse (the big boost) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Slide")
	float AirSlideImpulseMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Slide")
	float SlideGroundFriction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Slide")
	float SlideBrakingDecel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Slide")
	float SlideMomentumCost;

	/** Minimum momentum required for ground slide (can be less than air slide) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Slide")
	float GroundSlideMomentumCost;

	float SavedGroundFriction;
	float SavedBrakingDecel;
	float PendingSlideImpulse;
	FVector PendingSlideDirection;

	// ---- Wall Run ----
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|WallRun")
	bool bIsWallRunning;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|WallRun")
	bool bWallRunIsRightSide;

	/** Minimum speed to initiate wall run */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|WallRun")
	float MinSpeedToWallRun;

	/** Speed while wall running */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|WallRun")
	float WallRunSpeed;

	/** How far to trace for walls (from character center) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|WallRun")
	float WallRunTraceDistance;

	/** Gravity scale while wall running (0 = no gravity) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|WallRun")
	float WallRunGravityScale;

	/** How vertical the wall must be (1.0 = perfectly vertical, 0.9 = slight slope allowed) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|WallRun")
	float WallRunMinVerticalNormal;

	/** Horizontal impulse strength when wall jumping */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|WallRun")
	float WallJumpHorizontalStrength;

	/** Vertical impulse strength when wall jumping */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|WallRun")
	float WallJumpVerticalStrength;

	/** Camera roll angle when wall running (degrees) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|WallRun")
	float WallRunCameraTilt;

	FVector WallRunNormal;
	float SavedGravityScale;
	float WallRunCooldownTimer;

	// ---- Double Jump ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Jump")
	float DoubleJumpZVelocity;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|Jump")
	bool bHasDoubleJumped;

private:
	void StartGroundSlide();
	void StartAirSlide();
	void StopSlide();
	void StartCrouchWalk();
	void StopCrouchWalk();

	bool CheckWall(bool bCheckRight, FHitResult& OutHit);
	void StartWallRun(const FHitResult& WallHit, bool bIsRightSide);
	void StopWallRun();
	void UpdateWallRun(float DeltaTime);
};