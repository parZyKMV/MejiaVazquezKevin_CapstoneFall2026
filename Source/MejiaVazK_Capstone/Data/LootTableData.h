#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Data/ItemData.h"
#include "LootTableData.generated.h"

/** Una posible recompensa de la tabla. Weight = probabilidad relativa. */
USTRUCT(BlueprintType)
struct FLootEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot")
	TObjectPtr<UItemData> Item = nullptr;

	/** Mayor peso = sale mas seguido. Un peso de 0 nunca sale. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot", meta = (ClampMin = "0"))
	float Weight = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot", meta = (ClampMin = "1"))
	int32 MinCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot", meta = (ClampMin = "1"))
	int32 MaxCount = 1;
};

/** Resultado de tirar la tabla: un objeto y cuantos. */
USTRUCT(BlueprintType)
struct FLootDrop
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Loot")
	TObjectPtr<UItemData> Item = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Loot")
	int32 Count = 0;
};

/**
 * Tabla de botin de un cofre (o de un enemigo mas adelante).
 * Cada "tirada" elige un objeto segun su peso. El resultado cambia siempre (aleatorio normal).
 */
UCLASS(BlueprintType)
class MEJIAVAZK_CAPSTONE_API ULootTableData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	TArray<FLootEntry> Entries;

	/** Cuantas veces se tira la tabla por cofre (entre Min y Max). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "1"))
	int32 MinRolls = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "1"))
	int32 MaxRolls = 2;

	/** Tira la tabla. Si el mismo objeto sale dos veces, se suman las cantidades. */
	UFUNCTION(BlueprintCallable, Category = "Loot")
	void RollLoot(TArray<FLootDrop>& OutDrops) const;
};
