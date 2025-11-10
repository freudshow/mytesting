#include <stdio.h>
#include <stdlib.h>
#include <math.h>  // For M_PI and pow

// Forward declarations
typedef struct Shape Shape;
typedef struct Visitor Visitor;

// Visitor struct with function pointers for each concrete shape
struct Visitor {
    void (*visitCircle)(Visitor *self, void *circle);
    void (*visitSquare)(Visitor *self, void *square);
};

// Shape (Element) struct with accept function pointer
struct Shape {
    void (*accept)(Shape *self, Visitor *visitor);
};

// Concrete Shape: Circle
typedef struct {
    Shape base;  // "Inherit" from Shape
    double radius;
} Circle;

void circleAccept(Shape *self, Visitor *visitor)
{
    visitor->visitCircle(visitor, (Circle*) self);
}

Circle* createCircle(double radius)
{
    Circle *circle = malloc(sizeof(Circle));
    circle->base.accept = circleAccept;
    circle->radius = radius;
    return circle;
}

// Concrete Shape: Square
typedef struct {
    Shape base;  // "Inherit" from Shape
    double side;
} Square;

void squareAccept(Shape *self, Visitor *visitor)
{
    visitor->visitSquare(visitor, (Square*) self);
}

Square* createSquare(double side)
{
    Square *square = malloc(sizeof(Square));
    square->base.accept = squareAccept;
    square->side = side;
    return square;
}

// Concrete Visitor: AreaVisitor
typedef struct {
    Visitor base;  // "Inherit" from Visitor
    double area;   // State to accumulate or store result
} AreaVisitor;

void areaVisitCircle(Visitor *self, void *circlePtr)
{
    Circle *circle = (Circle*) circlePtr;
    AreaVisitor *av = (AreaVisitor*) self;
    av->area = M_PI * pow(circle->radius, 2);
    printf("Circle area: %.2f\n", av->area);
}

void areaVisitSquare(Visitor *self, void *squarePtr)
{
    Square *square = (Square*) squarePtr;
    AreaVisitor *av = (AreaVisitor*) self;
    av->area = pow(square->side, 2);
    printf("Square area: %.2f\n", av->area);
}

AreaVisitor* createAreaVisitor()
{
    AreaVisitor *av = malloc(sizeof(AreaVisitor));
    av->base.visitCircle = areaVisitCircle;
    av->base.visitSquare = areaVisitSquare;
    av->area = 0.0;
    return av;
}

// Concrete Visitor: PerimeterVisitor
typedef struct {
    Visitor base;  // "Inherit" from Visitor
    double perimeter;  // State to store result
} PerimeterVisitor;

void perimeterVisitCircle(Visitor *self, void *circlePtr)
{
    Circle *circle = (Circle*) circlePtr;
    PerimeterVisitor *pv = (PerimeterVisitor*) self;
    pv->perimeter = 2 * M_PI * circle->radius;
    printf("Circle perimeter: %.2f\n", pv->perimeter);
}

void perimeterVisitSquare(Visitor *self, void *squarePtr)
{
    Square *square = (Square*) squarePtr;
    PerimeterVisitor *pv = (PerimeterVisitor*) self;
    pv->perimeter = 4 * square->side;
    printf("Square perimeter: %.2f\n", pv->perimeter);
}

PerimeterVisitor* createPerimeterVisitor()
{
    PerimeterVisitor *pv = malloc(sizeof(PerimeterVisitor));
    pv->base.visitCircle = perimeterVisitCircle;
    pv->base.visitSquare = perimeterVisitSquare;
    pv->perimeter = 0.0;
    return pv;
}

// Example usage
void visitormain(void)
{
    // Create shapes
    Circle *circle = createCircle(5.0);
    Square *square = createSquare(4.0);

    // Create visitors
    AreaVisitor *areaVisitor = createAreaVisitor();
    PerimeterVisitor *perimeterVisitor = createPerimeterVisitor();

    // Apply visitors
    circle->base.accept((Shape*) circle, (Visitor*) areaVisitor);
    square->base.accept((Shape*) square, (Visitor*) areaVisitor);

    circle->base.accept((Shape*) circle, (Visitor*) perimeterVisitor);
    square->base.accept((Shape*) square, (Visitor*) perimeterVisitor);

    // Clean up (in real code, free all malloc'd memory)
    free(circle);
    free(square);
    free(areaVisitor);
    free(perimeterVisitor);
}
