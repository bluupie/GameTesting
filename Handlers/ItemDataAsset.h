#include <iostream>
#include <alogorithm>
#include "UObject/Class.h"

UCLASS(BlueprintType)
class INVENTORY_API UItemDataAsset : public UPrimaryDataAsset {
    GenerateBody();
};

public:
UPROPERTY()