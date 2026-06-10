// ALPuffEffect.cpp

#include "ALPuffEffect.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"

AALPuffEffect::AALPuffEffect()
{
	PrimaryActorTick.bCanEverTick = true;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComponent->SetCastShadow(false);
	RootComponent = MeshComponent;

	Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("Light"));
	Light->SetupAttachment(RootComponent);
	Light->SetCastShadows(false);
	Light->SetAttenuationRadius(300.f);
	Light->SetVisibility(false);

	Duration = 0.5f;
	StartScale = 0.05f;
	EndScale = 0.25f;
	ShapeScale = FVector::OneVector;
	Color = FLinearColor(0.4f, 0.38f, 0.36f);
	OpacityScale = 0.5f;
	DriftVelocity = FVector::ZeroVector;
	LightIntensity = 0.f;
	LightColor = FLinearColor(1.f, 0.55f, 0.2f);
	Age = 0.f;
}

void AALPuffEffect::BeginPlay()
{
	Super::BeginPlay();

	if (!PuffMesh)
	{
		PuffMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	}
	if (PuffMesh)
	{
		MeshComponent->SetStaticMesh(PuffMesh);
	}

	if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Weapons/Effects/M_PuffBall.M_PuffBall")))
	{
		MID = UMaterialInstanceDynamic::Create(Base, this);
		MID->SetVectorParameterValue(TEXT("Color"), Color);
		MID->SetScalarParameterValue(TEXT("OpacityScale"), OpacityScale);
		MID->SetScalarParameterValue(TEXT("Fade"), 1.f);
		MeshComponent->SetMaterial(0, MID);
	}

	SetActorScale3D(ShapeScale * StartScale);

	if (LightIntensity > 0.f)
	{
		Light->SetLightColor(LightColor);
		Light->SetIntensity(LightIntensity);
		Light->SetVisibility(true);
	}
}

void AALPuffEffect::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	Age += DeltaTime;
	const float T = FMath::Clamp(Age / FMath::Max(Duration, 0.01f), 0.f, 1.f);

	// Expand fast, settle toward the end
	const float Ease = 1.f - FMath::Square(1.f - T);
	SetActorScale3D(ShapeScale * FMath::Lerp(StartScale, EndScale, Ease));

	if (!DriftVelocity.IsNearlyZero())
	{
		AddActorWorldOffset(DriftVelocity * DeltaTime);
	}

	if (MID)
	{
		MID->SetScalarParameterValue(TEXT("Fade"), FMath::Pow(1.f - T, 1.5f));
	}

	if (LightIntensity > 0.f)
	{
		// Light dies faster than the puff so the flash reads as a pop
		Light->SetIntensity(LightIntensity * FMath::Cube(1.f - T));
	}

	if (T >= 1.f)
	{
		Destroy();
	}
}
