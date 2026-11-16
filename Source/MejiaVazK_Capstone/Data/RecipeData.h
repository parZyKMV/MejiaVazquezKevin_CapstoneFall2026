#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Data/ItemData.h"
#include "RecipeData.generated.h"

/** Un ingrediente de una receta: que objeto y cuantos. */
USTRUCT(BlueprintType)
struct FRecipeIngredient
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe")
	TObjectPtr<UItemData> Item = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe", meta = (ClampMin = "1"))
	int32 Count = 1;
};

/**
 * Una receta de cocina. Se crea como Data Asset, sin codigo.
 * Ingredientes -> un objeto resultado (normalmente un Consumible que cura vida o restaura mana).
 */
UCLASS(BlueprintType)
class MEJIAVAZK_CAPSTONE_API URecipeData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe", meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe")
	TArray<FRecipeIngredient> Ingredients;

	/** Objeto que se obtiene al cocinar. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe")
	TObjectPtr<UItemData> Result = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe", meta = (ClampMin = "1"))
	int32 ResultCount = 1;
};
