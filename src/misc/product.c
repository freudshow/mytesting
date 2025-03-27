#include <stdio.h>
#include <stdlib.h>

// Product interface
typedef struct {
    void (*operation)(void);
} Product;

// Concrete product
typedef struct {
    Product base;
} ConcreteProduct;

void concreteProductOperation(void) {
    printf("Concrete product operation\n");
}

// Creator interface
typedef struct {
    Product* (*createProduct)(void);
} Creator;

// Concrete creator
typedef struct {
    Creator base;
} ConcreteCreator;

Product* createConcreteProduct(void) {
    ConcreteProduct* product = (ConcreteProduct*)malloc(sizeof(ConcreteProduct));
    if (product == NULL) {
        // Handle memory allocation error
        return NULL;
    }

    // Initialize the product and set its operation function
    product->base.operation = concreteProductOperation;

    return &product->base;
}

int productmain() {
    // Client code uses the creator interface to create a product
    Creator* creator = (Creator*)malloc(sizeof(ConcreteCreator));
    if (creator == NULL) {
        // Handle memory allocation error
        return 1;
    }

    // Set the creator's createProduct function to the specific factory method
    creator->createProduct = createConcreteProduct;

    // Use the factory method to create a product
    Product* product = creator->createProduct();

    // Use the product
    if (product != NULL) {
        product->operation();
    }

    // Cleanup
    free(creator);
    free(product);

    return 0;
}
