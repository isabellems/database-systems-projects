//stack.h

typedef struct Node
{
	int id;
	struct Node *next;
	struct Node *previous;
}Node;

typedef struct Stack{
	Node* front;
	Node* rear;
	int count;
}Stack;

Stack *Stack_Create();

void Stack_push(int,Stack *);

int Stack_pop(Stack *);

void Stack_Destroy(Stack *);
