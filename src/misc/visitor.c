#include <stdio.h>
#include <stdlib.h>

// Forward declarations
typedef struct Element Element;
typedef struct Visitor Visitor;

// Function pointer type for accepting a visitor
typedef void (*AcceptFunc)(Element*, Visitor*);

// Basic element structure
struct Element {
    AcceptFunc accept;
};

// Concrete elements will inherit from this basic element

// ElementA structure and its accept function
typedef struct {
    Element base;
} ElementA;

void ElementA_Accept(Element *self, Visitor *visitor);

ElementA* CreateElementA()
{
    ElementA *element = (ElementA*) malloc(sizeof(ElementA));
    if (!element)
        return NULL;
    element->base.accept = ElementA_Accept;
    return element;
}

void DestroyElementA(ElementA *element)
{
    free(element);
}

// ElementB structure and its accept function
typedef struct {
    Element base;
} ElementB;

void ElementB_Accept(Element *self, Visitor *visitor);

ElementB* CreateElementB()
{
    ElementB *element = (ElementB*) malloc(sizeof(ElementB));
    if (!element)
        return NULL;
    element->base.accept = ElementB_Accept;
    return element;
}

void DestroyElementB(ElementB *element)
{
    free(element);
}

// Function pointers for visiting different elements
typedef void (*VisitElementAFunc)(Visitor*, ElementA*);
typedef void (*VisitElementBFunc)(Visitor*, ElementB*);

// Basic visitor structure
struct Visitor {
    VisitElementAFunc visitElementA;
    VisitElementBFunc visitElementB;
};

// ConcreteVisitor1 implementation
typedef struct {
    Visitor base;
} ConcreteVisitor1;

void ConcreteVisitor1_VisitElementA(Visitor *self, ElementA *element)
{
    printf("ConcreteVisitor1 visited ElementA\n");
}

void ConcreteVisitor1_VisitElementB(Visitor *self, ElementB *element)
{
    printf("ConcreteVisitor1 visited ElementB\n");
}

ConcreteVisitor1* CreateConcreteVisitor1()
{
    ConcreteVisitor1 *visitor = (ConcreteVisitor1*) malloc(sizeof(ConcreteVisitor1));
    if (!visitor)
        return NULL;
    visitor->base.visitElementA = ConcreteVisitor1_VisitElementA;
    visitor->base.visitElementB = ConcreteVisitor1_VisitElementB;
    return visitor;
}

void DestroyConcreteVisitor1(ConcreteVisitor1 *visitor)
{
    free(visitor);
}

// ConcreteVisitor2 implementation
typedef struct {
    Visitor base;
} ConcreteVisitor2;

void ConcreteVisitor2_VisitElementA(Visitor *self, ElementA *element)
{
    printf("ConcreteVisitor2 visited ElementA\n");
}

void ConcreteVisitor2_VisitElementB(Visitor *self, ElementB *element)
{
    printf("ConcreteVisitor2 visited ElementB\n");
}

ConcreteVisitor2* CreateConcreteVisitor2()
{
    ConcreteVisitor2 *visitor = (ConcreteVisitor2*) malloc(sizeof(ConcreteVisitor2));
    if (!visitor)
        return NULL;
    visitor->base.visitElementA = ConcreteVisitor2_VisitElementA;
    visitor->base.visitElementB = ConcreteVisitor2_VisitElementB;
    return visitor;
}

void DestroyConcreteVisitor2(ConcreteVisitor2 *visitor)
{
    free(visitor);
}

// Implementing accept functions for concrete elements
void ElementA_Accept(Element *self, Visitor *visitor)
{
    ElementA *element = (ElementA*) self;
    visitor->visitElementA(visitor, element);
}

void ElementB_Accept(Element *self, Visitor *visitor)
{
    ElementB *element = (ElementB*) self;
    visitor->visitElementB(visitor, element);
}

int visitormain()
{
    // Create elements
    ElementA *elementA = CreateElementA();
    ElementB *elementB = CreateElementB();

    // Create visitors
    ConcreteVisitor1 *visitor1 = CreateConcreteVisitor1();
    ConcreteVisitor2 *visitor2 = CreateConcreteVisitor2();

    // Use the accept method to visit each element with each visitor
    elementA->base.accept((Element*) elementA, (Visitor*) visitor1);
    elementB->base.accept((Element*) elementB, (Visitor*) visitor1);

    elementA->base.accept((Element*) elementA, (Visitor*) visitor2);
    elementB->base.accept((Element*) elementB, (Visitor*) visitor2);

    // Clean up memory
    DestroyElementA(elementA);
    DestroyElementB(elementB);
    DestroyConcreteVisitor1(visitor1);
    DestroyConcreteVisitor2(visitor2);

    return 0;
}
