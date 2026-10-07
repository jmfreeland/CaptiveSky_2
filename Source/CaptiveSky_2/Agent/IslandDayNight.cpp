#include "IslandDayNight.h"
#include "IslandWeather.h"
#include "IslandEnvironmentSubsystem.h"
#include "IslandWorldStateSubsystem.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"

AIslandDayNight::AIslandDayNight()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Moon = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Moon"));
	Moon->SetupAttachment(RootComponent);
	Moon->SetMobility(EComponentMobility::Movable);
	Moon->bAtmosphereSunLight = true;
	Moon->AtmosphereSunLightIndex = 1;
	Moon->SetLightColor(FLinearColor(0.55f, 0.68f, 1.f));
	Starlight = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Starlight"));
	Starlight->SetupAttachment(RootComponent);
	Starlight->SetMobility(EComponentMobility::Movable);
	Starlight->SetRelativeRotation(FRotator(-50.f, 215.f, 0.f));
	Starlight->SetLightColor(FLinearColor(0.5f, 0.62f, 1.f));
	Starlight->SetCastShadows(false);
	Starlight->SetIntensity(0.f);
	Starfield = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("NightStars"));
	Starfield->SetupAttachment(RootComponent);
	Starfield->SetMobility(EComponentMobility::Movable);
	Starfield->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Starfield->SetCanEverAffectNavigation(false);
	Starfield->SetCastShadow(false);
	Starfield->bReceivesDecals = false;
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> StarsMaterial(TEXT("/Engine/EngineDebugMaterials/VertexColorMaterial.VertexColorMaterial"));
	if (StarsMaterial.Succeeded()) StarMaterial = StarsMaterial.Object;
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.1f;
}

float AIslandDayNight::WrapHour(double Hour)
{
	return static_cast<float>(FMath::Fmod(FMath::Fmod(Hour, 24.0) + 24.0, 24.0));
}

float AIslandDayNight::SunHeight(float Hour)
{
	return FMath::Sin((WrapHour(Hour) - 6.f) * PI / 12.f);
}

float AIslandDayNight::NightAmount(float Hour)
{
	return FMath::SmoothStep(0.02f, 0.25f, -SunHeight(Hour));
}

float AIslandDayNight::TwilightFillAmount(float Hour)
{
	return 1.f - FMath::SmoothStep(-0.02f, 0.34f, SunHeight(Hour));
}

float AIslandDayNight::LunarPhaseProgress(int32 IslandDay, float IslandHour)
{
	const double ElapsedDays = static_cast<double>(FMath::Max(1, IslandDay) - 1) +
		WrapHour(IslandHour) / 24.0;
	return static_cast<float>(FMath::Fmod(ElapsedDays, LunarCycleDays) / LunarCycleDays);
}

float AIslandDayNight::LunarIllumination(int32 IslandDay, float IslandHour)
{
	const float PhaseAngle = LunarPhaseProgress(IslandDay, IslandHour) * 2.f * PI;
	return 0.5f - 0.5f * FMath::Cos(PhaseAngle);
}

float AIslandDayNight::MoonlightScale(float LunarIlluminationAmount)
{
	return FMath::Lerp(NewMoonLightFloor, 1.f, FMath::Clamp(LunarIlluminationAmount, 0.f, 1.f));
}

float AIslandDayNight::CloudSunlightTransmission(float CloudCover)
{
	return FMath::Lerp(1.f, 0.76f, FMath::Clamp(CloudCover, 0.f, 1.f));
}

float AIslandDayNight::CloudSkylightTransmission(float CloudCover)
{
	return FMath::Lerp(1.f, 0.70f, FMath::Clamp(CloudCover, 0.f, 1.f));
}

void AIslandDayNight::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	CurrentHour = WrapHour(StartHour);
	BuildStarfield();
	UpdateLighting();
}

void AIslandDayNight::BuildStarfield()
{
	if (!Starfield || !StarMaterial || Starfield->GetNumSections() > 0) return;
	TArray<FVector> Vertices[NightStarSectionCount];
	TArray<int32> Triangles[NightStarSectionCount];
	TArray<FVector> Normals[NightStarSectionCount];
	TArray<FVector2D> UVs[NightStarSectionCount];
	TArray<FColor> Colors[NightStarSectionCount];
	TArray<FProcMeshTangent> Tangents[NightStarSectionCount];
	FRandomStream Random(0x51A7F13);

	for (int32 StarIndex = 0; StarIndex < NightStarCount; ++StarIndex)
	{
		const float Z = Random.FRandRange(-1.f, 1.f);
		const float Azimuth = Random.FRandRange(0.f, 2.f * PI);
		const float Ring = FMath::Sqrt(FMath::Max(0.f, 1.f - Z * Z));
		const FVector Direction(Ring * FMath::Cos(Azimuth), Ring * FMath::Sin(Azimuth), Z);
		FVector Right = FVector::CrossProduct(Direction, FVector::UpVector).GetSafeNormal();
		if (Right.IsNearlyZero()) Right = FVector::CrossProduct(Direction, FVector::RightVector).GetSafeNormal();
		const FVector Up = FVector::CrossProduct(Right, Direction).GetSafeNormal();
		const FVector Normal = -Direction;
		// The previous 3.5–9 m half-sizes projected to mostly sub-pixel cards at
		// this 20 km shell, so even 1,200 stars disappeared in ordinary captures.
		const float HalfSize = Random.FRandRange(NightStarHalfSizeMin, NightStarHalfSizeMax);
		const FVector Center = Direction * NightStarShellRadius;
		const int32 SectionIndex = Random.RandRange(0, NightStarSectionCount - 1);
		const int32 FirstVertex = Vertices[SectionIndex].Num();
		Vertices[SectionIndex].Add(Center - Right * HalfSize - Up * HalfSize);
		Vertices[SectionIndex].Add(Center + Right * HalfSize - Up * HalfSize);
		Vertices[SectionIndex].Add(Center + Right * HalfSize + Up * HalfSize);
		Vertices[SectionIndex].Add(Center - Right * HalfSize + Up * HalfSize);
		Triangles[SectionIndex].Add(FirstVertex);
		Triangles[SectionIndex].Add(FirstVertex + 1);
		Triangles[SectionIndex].Add(FirstVertex + 2);
		Triangles[SectionIndex].Add(FirstVertex);
		Triangles[SectionIndex].Add(FirstVertex + 2);
		Triangles[SectionIndex].Add(FirstVertex + 3);
		for (int32 VertexIndex = 0; VertexIndex < 4; ++VertexIndex) Normals[SectionIndex].Add(Normal);
		UVs[SectionIndex].Add(FVector2D(0.f, 0.f));
		UVs[SectionIndex].Add(FVector2D(1.f, 0.f));
		UVs[SectionIndex].Add(FVector2D(1.f, 1.f));
		UVs[SectionIndex].Add(FVector2D(0.f, 1.f));
		const bool bWarmStar = Random.FRand() < 0.12f;
		const FLinearColor Tint = bWarmStar ? FLinearColor(1.f, 0.79f, 0.60f) : FLinearColor(0.72f, 0.86f, 1.f);
		const FColor Color = (Tint * Random.FRandRange(0.35f, 1.f)).ToFColor(true);
		for (int32 VertexIndex = 0; VertexIndex < 4; ++VertexIndex) Colors[SectionIndex].Add(Color);
		const FProcMeshTangent Tangent(Right, false);
		for (int32 VertexIndex = 0; VertexIndex < 4; ++VertexIndex) Tangents[SectionIndex].Add(Tangent);
	}

	for (int32 SectionIndex = 0; SectionIndex < NightStarSectionCount; ++SectionIndex)
	{
		Starfield->CreateMeshSection(SectionIndex, Vertices[SectionIndex], Triangles[SectionIndex], Normals[SectionIndex],
			UVs[SectionIndex], Colors[SectionIndex], Tangents[SectionIndex], false);
		Starfield->SetMaterial(SectionIndex, StarMaterial);
		Starfield->SetMeshSectionVisible(SectionIndex, false);
	}
}

void AIslandDayNight::UpdateStarfieldVisibility(float SolarElevation)
{
	if (!Starfield) return;
	constexpr float StarThresholds[NightStarSectionCount] = { -0.03f, -0.08f, -0.13f, -0.18f, -0.24f, -0.31f };
	for (int32 SectionIndex = 0; SectionIndex < NightStarSectionCount; ++SectionIndex)
		Starfield->SetMeshSectionVisible(SectionIndex, SolarElevation <= StarThresholds[SectionIndex]);
}

void AIslandDayNight::BeginPlay()
{
	Super::BeginPlay();
	CurrentHour = WrapHour(StartHour);
	DayNumber = 1;
	SecondsSinceSave = 0.f;
	if (const UIslandWorldStateSubsystem* WorldState = bResumeSavedTime ? GetWorld()->GetSubsystem<UIslandWorldStateSubsystem>() : nullptr)
	{
		if (const TOptional<float> Saved = WorldState->GetSavedHour(); Saved.IsSet()) CurrentHour = WrapHour(Saved.GetValue());
		DayNumber = WorldState->GetSavedDay().Get(1);
	}
	if (Sun) Sun->GetLightComponent()->SetMobility(EComponentMobility::Movable);
	if (Sky) Sky->GetLightComponent()->SetMobility(EComponentMobility::Movable);
	UpdateLighting();
}

#if WITH_EDITOR
void AIslandDayNight::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
	Super::PostEditChangeProperty(Event);
	if (!GetWorld() || !GetWorld()->IsGameWorld()) CurrentHour = WrapHour(StartHour);
	UpdateLighting();
}
#endif

void AIslandDayNight::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const float PreviousHour = CurrentHour;
	if (bAdvanceTime) CurrentHour = WrapHour(CurrentHour + FMath::Max(0.f, DeltaSeconds) * 24.0 / (FMath::Max(1.f, DayLengthMinutes) * 60.0));
	if (CurrentHour < PreviousHour) ++DayNumber;
	UpdateLighting();
	// Save occasionally too, so a crash or forced stop loses at most a minute of Island time.
	SecondsSinceSave += FMath::Max(0.f, DeltaSeconds);
	if (SecondsSinceSave >= 60.f) PersistHour();
}

void AIslandDayNight::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	PersistHour();
	Super::EndPlay(EndPlayReason);
}

void AIslandDayNight::PersistHour()
{
	SecondsSinceSave = 0.f;
	if (!GetWorld()) return;
	UIslandWorldStateSubsystem* WorldState = GetWorld()->GetSubsystem<UIslandWorldStateSubsystem>();
	if (!WorldState) return;
	if (bResumeSavedTime) WorldState->SaveClock(CurrentHour, DayNumber);
	if (const UIslandEnvironmentSubsystem* Environment = GetWorld()->GetSubsystem<UIslandEnvironmentSubsystem>())
		WorldState->SaveWetness(Environment->GetWetness());
}

void AIslandDayNight::UpdateLighting()
{
	const float Height = SunHeight(CurrentHour);
	const float Daylight = FMath::SmoothStep(-0.04f, 0.18f, Height);
	const float Angle = (CurrentHour - 6.f) * 15.f;
	float CloudCover = 0.f;
	if (GetWorld())
	{
		for (TActorIterator<AIslandWeather> It(GetWorld()); It; ++It)
		{
			CloudCover = It->SampleCloudCover(GetWorld()->GetTimeSeconds());
			break;
		}
	}
	if (Sun)
	{
		Sun->SetActorRotation(FRotator(-Angle, 35.f, 0.f));
		ULightComponent* Light = Sun->GetLightComponent();
		Light->SetIntensity(FMath::Max(0.f, DaySunIntensity) * Daylight * CloudSunlightTransmission(CloudCover));
		Light->SetLightColor(FMath::Lerp(FLinearColor(1.f, 0.32f, 0.12f), FLinearColor(1.f, 0.96f, 0.88f), FMath::SmoothStep(0.f, 0.4f, Height)));
	}
	const float LunarProgress = LunarPhaseProgress(DayNumber, CurrentHour);
	const float LunarIlluminationAmount = LunarIllumination(DayNumber, CurrentHour);
	UpdateStarfieldVisibility(Height);
	// New moon follows the sun below the horizon; full moon travels opposite it.
	// Intermediate phases therefore shift the moon's rise and set through the night.
	Moon->SetWorldRotation(FRotator(-Angle + LunarProgress * 360.f, 35.f, 0.f));
	Moon->SetIntensity(FMath::Max(0.f, MoonIntensity) * MoonlightScale(LunarIlluminationAmount) *
		NightAmount(CurrentHour));
	const float StarlightAmount = FMath::Max(NightAmount(CurrentHour), TwilightFillAmount(CurrentHour) * TwilightFillStrength);
	Starlight->SetIntensity(FMath::Max(0.f, StarlightIntensity) * StarlightAmount);
	if (Sky) Sky->GetLightComponent()->SetIntensity(FMath::Lerp(NightSkylightFloor, 1.f, Daylight) * CloudSkylightTransmission(CloudCover));

	// Translucent water and volumetric fog can use only one directional light. Give
	// the brightest current source a unique top priority; keep the inactive/secondary
	// sources ordered below it so ties never make the renderer choose by brightness.
	TArray<UDirectionalLightComponent*> DirectionalLights;
	if (Sun)
		if (UDirectionalLightComponent* SunComponent = Cast<UDirectionalLightComponent>(Sun->GetLightComponent()))
			DirectionalLights.Add(SunComponent);
	if (Moon) DirectionalLights.Add(Moon);
	if (Starlight) DirectionalLights.Add(Starlight);
	UDirectionalLightComponent* PrimaryLight = nullptr;
	for (UDirectionalLightComponent* Light : DirectionalLights)
		if (Light && (!PrimaryLight || Light->Intensity > PrimaryLight->Intensity)) PrimaryLight = Light;
	int32 SecondaryPriority = 0;
	for (UDirectionalLightComponent* Light : DirectionalLights)
		if (Light && Light != PrimaryLight)
		{
			if (Light->ForwardShadingPriority != SecondaryPriority)
				Light->SetForwardShadingPriority(SecondaryPriority);
			++SecondaryPriority;
		}
	if (PrimaryLight && PrimaryLight->ForwardShadingPriority != DirectionalLights.Num() - 1)
		PrimaryLight->SetForwardShadingPriority(DirectionalLights.Num() - 1);
}

FString AIslandDayNight::DescribeTime() const
{
	const TCHAR* Phase = CurrentHour < 5.f || CurrentHour >= 20.f ? TEXT("night") :
		CurrentHour < 7.f ? TEXT("dawn") : CurrentHour < 12.f ? TEXT("morning") :
		CurrentHour < 17.f ? TEXT("afternoon") : TEXT("dusk");
	const float LunarProgress = LunarPhaseProgress(DayNumber, CurrentHour);
	const TCHAR* LunarTrend = LunarProgress < 0.5f ? TEXT("waxing") : TEXT("waning");
	const float LunarLight = LunarIllumination(DayNumber, CurrentHour);
	const TCHAR* LunarBrightness = LunarLight < 0.12f ? TEXT("very faint") : LunarLight < 0.4f ? TEXT("faint") :
		LunarLight < 0.72f ? TEXT("moderate") : TEXT("bright");
	return FString::Printf(TEXT(" It is %s on the Island (approximately %02d:%02d). The lunar phase is %s with %s illumination; the moon's rise and set shift through the cycle. The sun follows its daily arc; cloud cover gently softens direct and ambient daylight."),
		Phase, FMath::FloorToInt(CurrentHour), FMath::FloorToInt(FMath::Frac(CurrentHour) * 60.f), LunarTrend, LunarBrightness);
}

static FAutoConsoleCommandWithWorldAndArgs GIslandHourCommand(
	TEXT("Island.Hour"),
	TEXT("Developer override for testing and filming: jump the Island clock to an hour (0..24). The day number is unchanged. Usage: Island.Hour <hour>"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (!World || Args.Num() < 1) return;
		for (TActorIterator<AIslandDayNight> It(World); It; ++It) It->CurrentHour = AIslandDayNight::WrapHour(FCString::Atof(*Args[0]));
	}));
