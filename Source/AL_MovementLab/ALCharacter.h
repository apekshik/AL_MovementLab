// ALCharacter.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ALCharacter.generated.h"

class UCameraComponent;
class UALCharacterMovementComponent;
class AALWeapon;
class UInputMappingContext;

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

	// Component whose relative Z gets the crouch/slide eye dip. When unset,
	// falls back to FirstPersonCamera. For viewmodel pawns set this to the
	// arms mesh so the socketed camera follows.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Crouch")
	TObjectPtr<USceneComponent> EyeHeightComponent;

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

	// Enhanced Input contexts swapped one tick after BeginPlay. Lets a child
	// blueprint replace a context its own BeginPlay graph adds (e.g. the FPS
	// Animation pack's full IMC) with a trimmed project-owned one, without
	// editing the blueprint graph.
	UPROPERTY(EditAnywhere, Category = "Input|Viewmodel")
	TArray<TSoftObjectPtr<UInputMappingContext>> ViewmodelContextsToRemove;

	UPROPERTY(EditAnywhere, Category = "Input|Viewmodel")
	TArray<TSoftObjectPtr<UInputMappingContext>> ViewmodelContextsToAdd;

	UPROPERTY(EditAnywhere, Category = "Input|Viewmodel")
	int32 ViewmodelContextPriority = 1;

	float BaseEyeZ;
	float CurrentEyeOffset;
	float TargetEyeOffset;
	float TargetCameraRoll;
	float CurrentCameraRoll;
	bool bIsCrouching;
	bool bWantsToMoveForward;

	USceneComponent* GetEyeComponent() const;
	void ApplyViewmodelInputContexts();

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

	// ---- Viewmodel (FPS Animation pack) ----

	// On-screen readout: active gun, fire mode, montage, movement state.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Viewmodel|Debug")
	bool bShowViewmodelDebug = true;

	// Flip each newly equipped pack weapon to full auto (pack default is
	// per-weapon; we want auto as the baseline).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Viewmodel")
	bool bForceAutoFireMode = true;

	UPROPERTY(Transient)
	TObjectPtr<AActor> LastFireModeWeapon;

	AActor* GetActiveViewmodelWeapon() const;
	void UpdateViewmodelDebug();
};