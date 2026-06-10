// ALWeapon.cpp

#include "ALWeapon.h"
#include "ALProjectile.h"
#include "ALPuffEffect.h"
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
	SightSocketName = TEXT("Sight");
	FireRate = 600.f;  // 600 RPM like an AR
	HipfireSpread = 2.f;  // 2 degrees spread when hipfiring
	AimTraceDistance = 50000.f;  // 500 meters
	bMuzzleFlash = true;
	MuzzleFlashScale = 1.f;
	TimeBetweenShots = 60.f / FireRate;
	TimeSinceLastShot = TimeBetweenShots;  // Can fire immediately
	bIsFiring = false;
	bIsADS = false;

	// Weapon positioning
	HipfireOffset = FVector(25.f, 16.f, -22.f);
	HipfireRotation = FRotator(0.f, -92.f, 0.f);  // slight cant toward screen center
	ADSRotation = FRotator(0.f, -90.f, 0.f);
	ADSOffset = FVector(25.f, 0.f, -20.f);  // fallback when no sight socket
	ADSSightDistance = 25.f;
	ADSInterpSpeed = 15.f;

	// Stow positioning
	StowedOffset = FVector(20.f, 20.f, -40.f);  // Down and to the side
	StowedRotation = FRotator(45.f, -90.f, 0.f);  // Rotated down
	StowInterpSpeed = 12.f;
	bIsStowed = false;

	// Recoil: dominated by the rearward shove along the barrel; only a small rise
	RecoilKick = FVector(-9.f, 0.f, 0.8f);
	RecoilRotation = FRotator(0.6f, 0.f, 0.f);
	RecoilYawRandom = 0.35f;
	RecoilRandomness = 0.25f;
	ADSRecoilScale = 0.55f;
	RecoilRecoverySpeed = 12.f;
	ViewKickPitch = 0.12f;
	ViewKickYaw = 0.06f;

	// Sway
	SwayScale = 0.018f;
	SwayMaxAngle = 3.f;
	SwayInterpSpeed = 10.f;
	SwayADSScale = 0.3f;

	CurrentRecoilOffset = FVector::ZeroVector;
	CurrentRecoilRotation = FRotator::ZeroRotator;
	CurrentSwayRotation = FRotator::ZeroRotator;
	LastOwnerControlRotation = FRotator::ZeroRotator;
	bSwayInitialized = false;
	BaseOffset = HipfireOffset;
	BaseRotation = HipfireRotation;
	ComputedADSOffset = ADSOffset;
	bHasSightSocket = false;
}

void AALWeapon::BeginPlay()
{
	Super::BeginPlay();

	// Recalculate in case FireRate was changed in editor
	TimeBetweenShots = 60.f / FireRate;

	BaseOffset = HipfireOffset;
	BaseRotation = HipfireRotation;

	// If the mesh has a sight socket, compute the ADS offset that puts it
	// exactly on the camera's center axis, ADSSightDistance in front.
	bHasSightSocket = WeaponMesh && WeaponMesh->DoesSocketExist(SightSocketName);
	if (bHasSightSocket)
	{
		const FVector SightLocal = WeaponMesh->GetSocketTransform(SightSocketName, RTS_Component).GetLocation();
		const FVector SightInCameraSpace = ADSRotation.RotateVector(SightLocal);
		ComputedADSOffset = FVector(ADSSightDistance, 0.f, 0.f) - SightInCameraSpace;
	}
	else
	{
		ComputedADSOffset = ADSOffset;
		UE_LOG(LogTemp, Warning,
			TEXT("%s: weapon mesh has no '%s' socket - ADS uses the fallback ADSOffset and the optic will not auto-center. Add the socket at the optic's lens center."),
			*GetName(), *SightSocketName.ToString());
	}
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

	UpdateSway(DeltaTime);

	// Determine base target position and rotation from state
	FVector TargetOffset;
	FRotator TargetRotation;
	float InterpSpeed;

	if (bIsStowed)
	{
		TargetOffset = StowedOffset;
		TargetRotation = StowedRotation;
		InterpSpeed = StowInterpSpeed;
	}
	else if (bIsADS)
	{
		TargetOffset = ComputedADSOffset;
		TargetRotation = ADSRotation;
		InterpSpeed = ADSInterpSpeed;
	}
	else
	{
		TargetOffset = HipfireOffset;
		TargetRotation = HipfireRotation;
		InterpSpeed = StowInterpSpeed;
	}

	// Interpolate the base transform only; recoil and sway are added on top
	// un-smoothed so kicks stay snappy instead of being interpolated away.
	BaseOffset = FMath::VInterpTo(BaseOffset, TargetOffset, DeltaTime, InterpSpeed);
	BaseRotation = FMath::RInterpTo(BaseRotation, TargetRotation, DeltaTime, InterpSpeed);

	SetActorRelativeLocation(BaseOffset + CurrentRecoilOffset);

	// Recoil/sway rotations are authored in camera space (pitch = muzzle up),
	// so pre-multiply them onto the base rotation rather than adding rotator
	// components, which would act around the mesh's own yawed axes.
	const FQuat OffsetQuat = (CurrentRecoilRotation + CurrentSwayRotation).Quaternion();
	WeaponMesh->SetRelativeRotation((OffsetQuat * BaseRotation.Quaternion()).Rotator());
}

void AALWeapon::UpdateSway(float DeltaTime)
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn || DeltaTime <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const FRotator ControlRot = OwnerPawn->GetControlRotation();
	if (!bSwayInitialized)
	{
		LastOwnerControlRotation = ControlRot;
		bSwayInitialized = true;
		return;
	}

	const float YawRate = FMath::FindDeltaAngleDegrees(LastOwnerControlRotation.Yaw, ControlRot.Yaw) / DeltaTime;
	const float PitchRate = FMath::FindDeltaAngleDegrees(LastOwnerControlRotation.Pitch, ControlRot.Pitch) / DeltaTime;
	LastOwnerControlRotation = ControlRot;

	// Weapon lags behind the look direction, then catches up
	const float Scale = SwayScale * (bIsADS ? SwayADSScale : 1.f);
	FRotator TargetSway;
	TargetSway.Yaw = FMath::Clamp(-YawRate * Scale, -SwayMaxAngle, SwayMaxAngle);
	TargetSway.Pitch = FMath::Clamp(-PitchRate * Scale, -SwayMaxAngle, SwayMaxAngle);
	TargetSway.Roll = TargetSway.Yaw * 0.3f;

	CurrentSwayRotation = FMath::RInterpTo(CurrentSwayRotation, TargetSway, DeltaTime, SwayInterpSpeed);
}

void AALWeapon::ApplyRecoil()
{
	const float Scale = (bIsADS ? ADSRecoilScale : 1.f)
		* FMath::FRandRange(1.f - RecoilRandomness, 1.f + RecoilRandomness);

	CurrentRecoilOffset += RecoilKick * Scale;

	FRotator Kick = RecoilRotation * Scale;
	Kick.Yaw += FMath::FRandRange(-RecoilYawRandom, RecoilYawRandom);
	CurrentRecoilRotation += Kick;

	// View kick: small permanent pitch climb the player compensates for
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (APlayerController* PC = OwnerPawn ? Cast<APlayerController>(OwnerPawn->GetController()) : nullptr)
	{
		FRotator ControlRot = PC->GetControlRotation();
		ControlRot.Pitch += ViewKickPitch * (bIsADS ? ADSRecoilScale : 1.f);
		ControlRot.Yaw += FMath::FRandRange(-ViewKickYaw, ViewKickYaw);
		PC->SetControlRotation(ControlRot);
	}
}

void AALWeapon::StartFire()
{
	if (bIsStowed)
	{
		return;
	}

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
	if (bIsStowed)
	{
		return;
	}

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

FVector AALWeapon::GetMuzzleLocation() const
{
	if (WeaponMesh && WeaponMesh->DoesSocketExist(MuzzleSocketName))
	{
		return WeaponMesh->GetSocketLocation(MuzzleSocketName);
	}

	// No socket: approximate a point in front of the weapon along the camera axis
	const USceneComponent* Parent = GetRootComponent() ? GetRootComponent()->GetAttachParent() : nullptr;
	const FVector Forward = Parent ? Parent->GetForwardVector() : GetActorForwardVector();
	return GetActorLocation() + Forward * 60.f;
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
	if (!ProjectileClass || bIsStowed)
	{
		return;
	}

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	APlayerController* PC = OwnerPawn ? Cast<APlayerController>(OwnerPawn->GetController()) : nullptr;

	// Always spawn from the muzzle and converge on the crosshair point,
	// hipfire and ADS alike — bullets visibly leave the barrel.
	const FVector SpawnLocation = GetMuzzleLocation();
	const FVector AimPoint = GetAimPoint(PC);

	FVector CameraForward = GetActorForwardVector();
	FVector CameraRight = GetActorRightVector();
	FVector CameraUp = GetActorUpVector();
	if (PC)
	{
		FVector CamLoc;
		FRotator CamRot;
		PC->GetPlayerViewPoint(CamLoc, CamRot);
		const FRotationMatrix CamMatrix(CamRot);
		CameraForward = CamMatrix.GetUnitAxis(EAxis::X);
		CameraRight = CamMatrix.GetUnitAxis(EAxis::Y);
		CameraUp = CamMatrix.GetUnitAxis(EAxis::Z);
	}

	FVector ShootDirection = (AimPoint - SpawnLocation).GetSafeNormal();

	// Crosshair on something closer than the barrel — fire along the camera instead
	if (FVector::DotProduct(ShootDirection, CameraForward) <= 0.f)
	{
		ShootDirection = CameraForward;
	}

	// Spread only when hipfiring
	if (!bIsADS && HipfireSpread > 0.f)
	{
		ShootDirection = FMath::VRandCone(ShootDirection, FMath::DegreesToRadians(HipfireSpread));
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

	if (FireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, SpawnLocation);
	}

	// Muzzle flash, attached to the weapon so it rides the recoil.
	// Hipfire: hot puff that starts small, expands fast and dies in ~70ms.
	// ADS: two sharp streaks flaring out to the sides so the optic stays clear.
	if (bMuzzleFlash)
	{
		if (bIsADS)
		{
			for (int32 Side = -1; Side <= 1; Side += 2)
			{
				const float SizeRand = FMath::FRandRange(0.85f, 1.25f);
				const FVector StreakDir = (CameraRight * Side
					+ CameraForward * FMath::FRandRange(0.05f, 0.25f)
					+ CameraUp * FMath::FRandRange(-0.15f, 0.1f)).GetSafeNormal();
				const FTransform StreakTransform(StreakDir.Rotation(), SpawnLocation + StreakDir * 14.f);
				if (AALPuffEffect* Streak = GetWorld()->SpawnActorDeferred<AALPuffEffect>(AALPuffEffect::StaticClass(), StreakTransform, this))
				{
					Streak->Duration = 0.05f;
					Streak->StartScale = 0.05f * MuzzleFlashScale * SizeRand;
					Streak->EndScale = 0.14f * MuzzleFlashScale * SizeRand;
					Streak->ShapeScale = FVector(3.2f, 0.3f, 0.3f);  // long thin spike along the streak direction
					Streak->Color = FLinearColor(40.f, 14.f, 2.5f);
					Streak->OpacityScale = 0.85f;
					Streak->LightIntensity = (Side > 0) ? 600.f : 0.f;  // one light is plenty
					Streak->LightColor = FLinearColor(1.f, 0.55f, 0.2f);
					Streak->FinishSpawning(StreakTransform);
					Streak->AttachToComponent(WeaponMesh, FAttachmentTransformRules::KeepWorldTransform);
				}
			}
		}
		else
		{
			const float SizeRand = FMath::FRandRange(0.8f, 1.3f);
			const FTransform FlashTransform(FRotator::ZeroRotator, SpawnLocation);
			if (AALPuffEffect* Flash = GetWorld()->SpawnActorDeferred<AALPuffEffect>(AALPuffEffect::StaticClass(), FlashTransform, this))
			{
				Flash->Duration = 0.07f;
				Flash->StartScale = 0.05f * MuzzleFlashScale * SizeRand;
				Flash->EndScale = 0.26f * MuzzleFlashScale * SizeRand;
				Flash->Color = FLinearColor(40.f, 14.f, 2.5f);
				Flash->OpacityScale = 0.9f;
				Flash->LightIntensity = 1500.f;
				Flash->LightColor = FLinearColor(1.f, 0.55f, 0.2f);
				Flash->FinishSpawning(FlashTransform);
				Flash->AttachToComponent(WeaponMesh, FAttachmentTransformRules::KeepWorldTransform);
			}
		}
	}

	// Apply visual recoil
	ApplyRecoil();

	// Reset shot timer
	TimeSinceLastShot = 0.f;
}
