// ALWeapon.cpp

#include "ALWeapon.h"
#include "ALProjectile.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

AALWeapon::AALWeapon()
{
	PrimaryActorTick.bCanEverTick = true;

	// Create weapon mesh
	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	RootComponent = WeaponMesh;

	// Default settings
	MuzzleSocketName = TEXT("Muzzle");
	FireRate = 600.f;  // 600 RPM like an AR
	HipfireSpread = 2.f;  // 2 degrees spread when hipfiring
	AimTraceDistance = 50000.f;  // 500 meters
	TimeBetweenShots = 60.f / FireRate;
	TimeSinceLastShot = TimeBetweenShots;  // Can fire immediately
	bIsFiring = false;
	bIsADS = false;

	// Weapon positioning
	HipfireOffset = FVector(30.f, 20.f, -15.f);
	ADSOffset = FVector(30.f, 0.f, -16.f);  // Centered
	ADSInterpSpeed = 15.f;

	// Stow positioning
	StowedOffset = FVector(20.f, 20.f, -40.f);  // Down and to the side
	StowedRotation = FRotator(45.f, -90.f, 0.f);  // Rotated down
	StowInterpSpeed = 12.f;
	bIsStowed = false;

	// Recoil
	RecoilKick = FVector(-3.f, 0.f, 1.f);  // Kick back and up
	RecoilRotation = FRotator(-2.f, 0.f, 0.f);  // Pitch up
	RecoilRecoverySpeed = 15.f;
	CurrentRecoilOffset = FVector::ZeroVector;
	CurrentRecoilRotation = FRotator::ZeroRotator;
}

void AALWeapon::BeginPlay()
{
	Super::BeginPlay();

	// Recalculate in case FireRate was changed in editor
	TimeBetweenShots = 60.f / FireRate;
}

void AALWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	TimeSinceLastShot += DeltaTime;

	// Auto-fire if trigger is held (only if not stowed)
	if (bIsFiring && !bIsStowed && TimeSinceLastShot >= TimeBetweenShots)
	{
		Fire();
	}

	// Recover recoil
	CurrentRecoilOffset = FMath::VInterpTo(CurrentRecoilOffset, FVector::ZeroVector, DeltaTime, RecoilRecoverySpeed);
	CurrentRecoilRotation = FMath::RInterpTo(CurrentRecoilRotation, FRotator::ZeroRotator, DeltaTime, RecoilRecoverySpeed);

	// Determine target position and rotation based on state
	FVector TargetOffset;
	FRotator TargetRotation;
	float InterpSpeed;

	if (bIsStowed)
	{
		TargetOffset = StowedOffset;
		TargetRotation = StowedRotation;
		InterpSpeed = StowInterpSpeed;
	}
	else
	{
		TargetOffset = (bIsADS ? ADSOffset : HipfireOffset) + CurrentRecoilOffset;
		TargetRotation = FRotator(0.f, -90.f, 0.f) + CurrentRecoilRotation;
		InterpSpeed = bIsADS ? ADSInterpSpeed : StowInterpSpeed;
	}

	// Interpolate position
	FVector CurrentOffset = GetRootComponent()->GetRelativeLocation();
	FVector NewOffset = FMath::VInterpTo(CurrentOffset, TargetOffset, DeltaTime, InterpSpeed);
	SetActorRelativeLocation(NewOffset);

	// Interpolate rotation
	FRotator CurrentMeshRot = WeaponMesh->GetRelativeRotation();
	WeaponMesh->SetRelativeRotation(FMath::RInterpTo(CurrentMeshRot, TargetRotation, DeltaTime, InterpSpeed));
}

void AALWeapon::ApplyRecoil()
{
	CurrentRecoilOffset += RecoilKick;
	CurrentRecoilRotation += RecoilRotation;
}

void AALWeapon::StartFire()
{
	bIsFiring = true;

	// Fire immediately if ready
	if (TimeSinceLastShot >= TimeBetweenShots)
	{
		Fire();
	}
}

void AALWeapon::StopFire()
{
	bIsFiring = false;
}

void AALWeapon::StartADS()
{
	bIsADS = true;
}

void AALWeapon::StopADS()
{
	bIsADS = false;
}

void AALWeapon::Stow()
{
	bIsStowed = true;
	bIsFiring = false;  // Stop firing when stowing
	bIsADS = false;     // Exit ADS when stowing
}

void AALWeapon::Draw()
{
	bIsStowed = false;
}

FVector AALWeapon::GetAimPoint(APlayerController* PC) const
{
	if (!PC)
	{
		return GetActorLocation() + GetActorForwardVector() * AimTraceDistance;
	}

	FVector CameraLocation;
	FRotator CameraRotation;
	PC->GetPlayerViewPoint(CameraLocation, CameraRotation);

	FVector TraceEnd = CameraLocation + CameraRotation.Vector() * AimTraceDistance;

	// Line trace to find what the crosshair is pointing at
	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(GetOwner());

	if (GetWorld()->LineTraceSingleByChannel(HitResult, CameraLocation, TraceEnd, ECC_Visibility, QueryParams))
	{
		return HitResult.ImpactPoint;
	}

	return TraceEnd;
}

void AALWeapon::Fire()
{
	if (!ProjectileClass)
	{
		return;
	}

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	APlayerController* PC = OwnerPawn ? Cast<APlayerController>(OwnerPawn->GetController()) : nullptr;

	FVector SpawnLocation;
	FVector ShootDirection;

	if (bIsADS)
	{
		// ADS: Spawn from camera, shoot straight
		if (PC)
		{
			FVector CameraLocation;
			FRotator CameraRotation;
			PC->GetPlayerViewPoint(CameraLocation, CameraRotation);

			SpawnLocation = CameraLocation + CameraRotation.Vector() * 50.f;
			ShootDirection = CameraRotation.Vector();
		}
		else
		{
			SpawnLocation = GetActorLocation();
			ShootDirection = GetActorForwardVector();
		}
	}
	else
	{
		// Hipfire: Spawn from barrel tip (approximated offset)
		// Use camera vectors since weapon is rotated
		FVector CameraForward, CameraRight, CameraUp;
		if (PC)
		{
			FRotator CamRot;
			FVector CamLoc;
			PC->GetPlayerViewPoint(CamLoc, CamRot);
			CameraForward = CamRot.Vector();
			CameraRight = FRotationMatrix(CamRot).GetUnitAxis(EAxis::Y);
			CameraUp = FRotationMatrix(CamRot).GetUnitAxis(EAxis::Z);
		}
		else
		{
			CameraForward = GetActorForwardVector();
			CameraRight = GetActorRightVector();
			CameraUp = GetActorUpVector();
		}

		SpawnLocation = GetActorLocation()
			+ CameraForward * 25.f
			+ CameraRight * 30.f
			+ CameraUp * 0.f;

		// Get aim point (where crosshair is pointing in world)
		FVector AimPoint = GetAimPoint(PC);

		// Direction from muzzle to aim point
		ShootDirection = (AimPoint - SpawnLocation).GetSafeNormal();

		// Add spread
		if (HipfireSpread > 0.f)
		{
			float SpreadRad = FMath::DegreesToRadians(HipfireSpread);
			ShootDirection = FMath::VRandCone(ShootDirection, SpreadRad);
		}
	}

	// Spawn projectile
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwner();
	SpawnParams.Instigator = Cast<APawn>(GetOwner());

	FRotator SpawnRotation = ShootDirection.Rotation();

	AALProjectile* Projectile = GetWorld()->SpawnActor<AALProjectile>(
		ProjectileClass,
		SpawnLocation,
		SpawnRotation,
		SpawnParams
	);

	if (Projectile)
	{
		Projectile->FireInDirection(ShootDirection);
	}

	// Apply visual recoil
	ApplyRecoil();

	// Reset shot timer
	TimeSinceLastShot = 0.f;
}
