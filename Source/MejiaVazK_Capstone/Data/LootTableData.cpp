#include "Data/LootTableData.h"

void ULootTableData::RollLoot(TArray<FLootDrop>& OutDrops) const
{
	OutDrops.Reset();

	float TotalWeight = 0.f;
	for (const FLootEntry& Entry : Entries)
	{
		if (Entry.Item && Entry.Weight > 0.f)
		{
			TotalWeight += Entry.Weight;
		}
	}

	if (TotalWeight <= 0.f)
	{
		return;
	}

	const int32 Rolls = FMath::RandRange(FMath::Max(1, MinRolls), FMath::Max(FMath::Max(1, MinRolls), MaxRolls));

	for (int32 Roll = 0; Roll < Rolls; ++Roll)
	{
		float Pick = FMath::FRandRange(0.f, TotalWeight);
		const FLootEntry* Chosen = nullptr;

		for (const FLootEntry& Entry : Entries)
		{
			if (!Entry.Item || Entry.Weight <= 0.f)
			{
				continue;
			}

			Chosen = &Entry; // por si el redondeo deja a Pick sin caer en ninguna
			Pick -= Entry.Weight;
			if (Pick <= 0.f)
			{
				break;
			}
		}

		if (!Chosen)
		{
			continue;
		}

		const int32 Count = FMath::RandRange(FMath::Max(1, Chosen->MinCount), FMath::Max(FMath::Max(1, Chosen->MinCount), Chosen->MaxCount));

		FLootDrop* Existing = OutDrops.FindByPredicate([Chosen](const FLootDrop& Drop) { return Drop.Item == Chosen->Item; });
		if (Existing)
		{
			Existing->Count += Count;
		}
		else
		{
			FLootDrop NewDrop;
			NewDrop.Item = Chosen->Item;
			NewDrop.Count = Count;
			OutDrops.Add(NewDrop);
		}
	}
}
