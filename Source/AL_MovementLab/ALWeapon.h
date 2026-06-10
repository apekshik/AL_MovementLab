// ALWeapon.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ALWeapon.generated.h"

class USkeletalMeshComponent;
class USoundBase;
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

	/** Stow/draw weapon */
	void Stow();
	void Draw();

	bool IsFiring() const { return bIsFiring; }
	bool IsADS() const { return bIsADS; }
	bool IsStowed() const { return bIsStowed; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<USkeletalMeshComponent> WeaponMesh;

	/** Projectile class to spawn */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	TSubclassOf<AALProjectile> ProjectileClass;

	/** Muzzle socket on the weapon mesh; projectiles spawn from here */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	FName MuzzleSocketName;

	/** Sight socket on the weapon mesh; auto-aligned to screen center during ADS if it exists */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	FName SightSocketName;

	/** Optional sound played per shot */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	TObjectPtr<USoundBase> FireSound;

	/** Fire rate in rounds per minute */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float FireRate;

	/** Hipfire spread in degrees */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float HipfireSpread;

	/** Max distance for aim trace */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float AimTraceDistance;

	/** Spawn a muzzle flash puff per shot */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|MuzzleFlash")
	bool bMuzzleFlash;

	/** Overall muzzle flash size multiplier */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|MuzzleFlash")
	float MuzzleFlashScale;

	// ---- Position ----

	/** Weapon position when hipfiring (relative to camera: X forward, Y right, Z up) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Position")
	FVector HipfireOffset;

	/** Weapon rotation when hipfiring. Yaw -90 points the barrel forward; a couple extra degrees of yaw cants the muzzle toward screen center */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Position")
	FRotator HipfireRotation;

	/** Weapon rotation when ADS (must point the barrel straight forward) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Position")
	FRotator ADSRotation;

	/** Fallback ADS position, used only when the mesh has no sight socket */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Position")
	FVector ADSOffset;

	/** How far in front of the camera the sight socket sits during ADS */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Position")
	float ADSSightDistance;

	/** How fast weapon moves between hipfire and ADS positions */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Position")
	float ADSInterpSpeed;

	/** Weapon position when stowed (off screen) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Position")
	FVector StowedOffset;

	/** Weapon rotation when stowed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Position")
	FRotator StowedRotation;

	/** How fast weapon stows/draws */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Position")
	float StowInterpSpeed;

	/** Is weapon stowed */
	bool bIsStowed;

	// ---- Recoil ----

	/** Positional kick per shot in camera space (X back, Y right, Z up) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Recoil")
	FVector RecoilKick;

	/** Rotational kick per shot in camera space (positive pitch = muzzle up) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Recoil")
	FRotator RecoilRotation;

	/** Random yaw added per shot, +/- this many degrees */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Recoil")
	float RecoilYawRandom;

	/** Random variation of kick strength per shot, 0..1 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Recoil")
	float RecoilRandomness;

	/** Recoil multiplier while ADS */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Recoil")
	float ADSRecoilScale;

	/** How fast recoil recovers */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Recoil")
	float RecoilRecoverySpeed;

	/** Camera pitch climb per shot, in degrees (player must pull down to compensate) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Recoil")
	float ViewKickPitch;

	/** Random camera yaw per shot, +/- this many degrees */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Recoil")
	float ViewKickYaw;

	// ---- Sway ----

	/** Degrees of weapon lag per (degree/sec) of look speed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Sway")
	float SwayScale;

	/** Max sway angle in degrees */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Sway")
	float SwayMaxAngle;

	/** How fast sway catches up to the camera */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Sway")
	float SwayInterpSpeed;

	/** Sway multiplier while ADS */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Sway")
	float SwayADSScale;

	// ---- Runtime state ----

	/** Current recoil offset being applied */
	FVector CurrentRecoilOffset;

	/** Current recoil rotation being applied */
	FRotator CurrentRecoilRotation;

	/** Current sway rotation being applied */
	FRotator CurrentSwayRotation;

	/** Owner's control rotation last frame, for sway rate */
	FRotator LastOwnerControlRotation;
	bool bSwayInitialized;

	/** Interpolated base transform (recoil/sway are added on top) */
	FVector BaseOffset;
	FRotator BaseRotation;

	/** ADS offset computed from the sight socket at BeginPlay */
	FVector ComputedADSOffset;
	bool bHasSightSocket;

	/** Apply recoil kick */
	void ApplyRecoil();

	/** Update look-lag sway */
	void UpdateSway(float DeltaTime);

	/** World-space muzzle location (socket if present, else in front of the weapon) */
	FVector GetMuzzleLocation() const;

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
