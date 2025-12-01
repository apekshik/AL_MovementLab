// ALWeapon.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ALWeapon.generated.h"

class USkeletalMeshComponent;
class AALProjectile;

UCLASS()
class AL_MOVEMENTLAB_API AALWeapon : public AActor
{
	GENERATED_BODY()

public:
	AALWeapon();

	/** Fire the weapon */
	void Fire();

	/** Start/stop automatic fire */
	void StartFire();
	void StopFire();

	/** ADS state */
	void StartADS();
	void StopADS();

	bool IsFiring() const { return bIsFiring; }
	bool IsADS() const { return bIsADS; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<USkeletalMeshComponent> WeaponMesh;

	/** Projectile class to spawn */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	TSubclassOf<AALProjectile> ProjectileClass;

	/** Muzzle socket name on the weapon mesh */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	FName MuzzleSocketName;

	/** Fire rate in rounds per minute */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float FireRate;

	/** Hipfire spread in degrees */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float HipfireSpread;

	/** Max distance for aim trace */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float AimTraceDistance;

	/** Weapon position when hipfiring (relative to camera) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Position")
	FVector HipfireOffset;

	/** Weapon position when ADS (relative to camera) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Position")
	FVector ADSOffset;

	/** How fast weapon moves between hipfire and ADS positions */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Position")
	float ADSInterpSpeed;

	/** Time between shots */
	float TimeBetweenShots;

	/** Time since last shot */
	float TimeSinceLastShot;

	/** Is the trigger held */
	bool bIsFiring;

	/** Is aiming down sights */
	bool bIsADS;

	/** Get the aim point in world space (where crosshair is pointing) */
	FVector GetAimPoint(APlayerController* PC) const;
};
