#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IslandGuestBook.generated.h"

class UTextRenderComponent;
class UStaticMeshComponent;
class UMaterialInterface;

/** Transient, code-built open book on the inn counter; its page shows recent persisted entries. */
UCLASS()
class CAPTIVESKY_2_API AIslandGuestBook : public AActor
{
	GENERATED_BODY()

public:
	AIslandGuestBook();

	void SetDisplayText(const FString& Text);
	FString GetDisplayText() const;
	bool HasCollision() const;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Island|Guest Book")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Island|Guest Book")
	TObjectPtr<UStaticMeshComponent> Cover;

	UPROPERTY(VisibleAnywhere, Category = "Island|Guest Book")
	TObjectPtr<UStaticMeshComponent> LeftPage;

	UPROPERTY(VisibleAnywhere, Category = "Island|Guest Book")
	TObjectPtr<UStaticMeshComponent> RightPage;

	UPROPERTY(VisibleAnywhere, Category = "Island|Guest Book")
	TObjectPtr<UStaticMeshComponent> Spine;

	UPROPERTY(VisibleAnywhere, Category = "Island|Guest Book")
	TObjectPtr<UTextRenderComponent> TitleText;

	UPROPERTY(VisibleAnywhere, Category = "Island|Guest Book")
	TObjectPtr<UTextRenderComponent> PageText;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> ShapeMaterial;

	FString CurrentDisplayText;
};
