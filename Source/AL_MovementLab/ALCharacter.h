// ALCharacter.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ALCharacter.generated.h"

class UCameraComponent;
class UALCharacterMovementComponent;
class AALWeapon;

UCLASS()
class AL_MOVEMENTLAB_API AALCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AALCharacter(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UALCharacterMovementComponent* GetALMovementComponent() const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> FirstPersonCamera;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Crouch")
	float StandingCameraHeight = 64.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Crouch")
	float CrouchingCameraHeight = 32.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Crouch")
	float CrouchCameraInterpSpeed = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|WallRun")
	float WallRunCameraTiltInterpSpeed = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|ADS")
	float DefaultFOV = 90.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|ADS")
	float ADSFOV = 72.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|ADS")
	float ADSFOVInterpSpeed = 12.f;

	float TargetCameraHeight;
	float TargetCameraRoll;
	float CurrentCameraRoll;
	bool bIsCrouching;
	bool bWantsToMoveForward;

	// ---- Input Handlers ----
	void MoveForward(float Value);
	void MoveRight(float Value);
	void Turn(float Value);
	void LookUp(float Value);

	void StartSprint();
	void StopSprint();

	void StartCrouch();
	void StopCrouch();

	void OnJumpPressed();
	void OnJumpReleased();

	void UpdateCameraTilt(float DeltaTime);

	// ---- Weapon ----
	void StartFire();
	void StopFire();
	void StartADS();
	void StopADS();
	void DrawWeapon();
	void StowWeapon();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	TSubclassOf<AALWeapon> WeaponClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<AALWeapon> CurrentWeapon;

	void SpawnWeapon();
};