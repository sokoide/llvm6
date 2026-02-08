struct Node {
    int value;
    struct Node* next;
};

int main() {
    struct Node n1;
    struct Node n2;
    struct Node* ptr = &n1;
    
    n1.value = 100;
    n1.next = &n2;
    n2.value = 200;
    
    return ptr->value + ptr->next->value;
}
