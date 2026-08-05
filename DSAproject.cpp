  #include <iostream>
#include <fstream>
#include <cstring>
using namespace std;

// =================== GLOBAL LOG ===================
ofstream logFile;

// =================== PRODUCT (BST) ===================
struct Product {
    int id;
    char name[50];
    int qty;
    int shelf;
    int freq;  
    Product *left, *right;

    Product(int i=0, const char* n="", int q=0, int s=0){
        id=i; strcpy(name,n); qty=q; shelf=s; freq=0;
        left=right=NULL;
    }
};

Product* proot=NULL;
int warehouseStock=1000;

// Insert BST
Product* insertProduct(Product* r, int id, const char* name, int qty, int shelf){
    if(!r) return new Product(id,name,qty,shelf);
    if(id < r->id) r->left = insertProduct(r->left,id,name,qty,shelf);
    else if(id>r->id) r->right = insertProduct(r->right,id,name,qty,shelf);
    else r->qty+=qty;  
    return r;
}

// Search BST
Product* searchProduct(Product* r, int id){
    if(!r) return NULL;
    if(id==r->id) return r;
    if(id<r->id) return searchProduct(r->left,id);
    return searchProduct(r->right,id);
}

// Display all products
void displayProducts(Product* r){
    if(!r) return;
    displayProducts(r->left);
    cout<<r->id<<" | "<<r->name<<" | Qty: "<<r->qty<<" | Shelf "<<r->shelf<<" | Freq "<<r->freq<<endl;
    displayProducts(r->right);
}

// Prefix search
void prefixSearch(Product* r, const char* prefix){
    if(!r) return;
    prefixSearch(r->left,prefix);
    if(strncmp(r->name,prefix,strlen(prefix))==0)
        cout<<r->id<<" | "<<r->name<<" | Qty: "<<r->qty<<" | Freq "<<r->freq<<endl;
    prefixSearch(r->right,prefix);
}

// Delete Product
Product* deleteProduct(Product* r, int id){
    if(!r) return NULL;
    if(id<r->id) r->left = deleteProduct(r->left,id);
    else if(id>r->id) r->right = deleteProduct(r->right,id);
    else{
        if(!r->left){ Product* tmp=r->right; delete r; return tmp; }
        else if(!r->right){ Product* tmp=r->left; delete r; return tmp; }
        else{
            Product* succ=r->right;
            while(succ->left) succ=succ->left;
            r->id=succ->id;
            strcpy(r->name,succ->name);
            r->qty=succ->qty;
            r->shelf=succ->shelf;
            r->freq=succ->freq;
            r->right=deleteProduct(r->right,succ->id);
        }
    }
    return r;
}

// =================== UNDO STACK ===================
struct UndoNode {
    int id, qty;
    UndoNode* next;
    UndoNode(int i,int q){ id=i; qty=q; next=NULL; }
};
UndoNode* undoTop=NULL;

void recordUndo(int id,int qty){
    UndoNode* u=new UndoNode(id,qty);
    u->next=undoTop;
    undoTop=u;
}

void undoProductAction(){
    if(!undoTop){ cout<<"No product actions to undo.\n"; return; }
    UndoNode* u=undoTop;
    undoTop=undoTop->next;

    Product* p=searchProduct(proot,u->id);
    if(p){
        int undoQty = u->qty;
        if(undoQty > p->qty) undoQty = p->qty;
        p->qty -= undoQty;
        warehouseStock += undoQty;
        cout<<"Undo: Returned "<<undoQty<<" units of "<<p->name<<endl;
        logFile<<"Undo: Returned "<<undoQty<<" units of "<<p->name<<endl;
    }
    delete u;
}

// =================== PICKERS ===================
struct Picker {
    int id;
    char name[30];
    bool busy;
    int pendingOrders;
    Picker* next;
    Picker(int i,const char* n){ id=i; strcpy(name,n); busy=false; pendingOrders=0; next=NULL; }
};
Picker* plist=NULL;

void addPicker(int id,const char* name){
    Picker* p=new Picker(id,name);
    p->next=plist;
    plist=p;
}

// Get least loaded free picker
Picker* getLeastLoadedPicker(){
    Picker* best=NULL;
    for(Picker* p=plist;p;p=p->next){
        if(!p->busy){
            if(!best || p->pendingOrders<best->pendingOrders)
                best=p;
        }
    }
    return best;
}

// Reset pickers after completing some orders
void completePickerTasks(){
    for(Picker* p=plist;p;p=p->next){
        if(p->pendingOrders>0){
            p->pendingOrders--;
            if(p->pendingOrders==0) p->busy=false;
        }
    }
}

// =================== MAX HEAP (Orders) ===================
struct Order {
    int oid, pid, qty, priority;
    char customerName[50];
    char customerContact[50];
    bool completed;
};

Order* heapArr[100];
int heapSize=0;

void swapOrder(Order* &a, Order* &b){ Order* t=a; a=b; b=t; }

void heapifyUp(int i){
    while(i>1){
        int parent=i/2;
        if(heapArr[i]->priority>heapArr[parent]->priority){
            swapOrder(heapArr[i],heapArr[parent]);
            i=parent;
        } else break;
    }
}

void heapifyDown(int i){
    while(true){
        int left=i*2, right=i*2+1, largest=i;
        if(left<=heapSize && heapArr[left]->priority>heapArr[largest]->priority) largest=left;
        if(right<=heapSize && heapArr[right]->priority>heapArr[largest]->priority) largest=right;
        if(largest!=i){ swapOrder(heapArr[i],heapArr[largest]); i=largest; } else break;
    }
}

void heapInsert(Order* o){
    heapSize++;
    heapArr[heapSize]=o;
    heapifyUp(heapSize);
}

Order* heapRemoveMax(){
    if(heapSize==0) return NULL;
    Order* top=heapArr[1];
    heapArr[1]=heapArr[heapSize];
    heapSize--;
    heapifyDown(1);
    return top;
}

// =================== ORDER UNDO STACK ===================
struct OrderUndo {
    int pid, qtyUndo;
    OrderUndo* next;
    OrderUndo(int p,int q){ pid=p; qtyUndo=q; next=NULL; }
};
OrderUndo* orderUndoTop=NULL;

void recordOrderUndo(int pid,int qty){
    OrderUndo* u=new OrderUndo(pid,qty);
    u->next=orderUndoTop;
    orderUndoTop=u;
}

void showOrderUndoList(){
    if(!orderUndoTop){ cout<<"No order actions to undo.\n"; return; }
    cout<<"\n=== Undoable Orders ===\n";
    OrderUndo* temp=orderUndoTop;
    int i=1;
    while(temp){
        Product* p=searchProduct(proot,temp->pid);
        if(p) cout<<i<<". Product: "<<p->name<<" | Undo Qty Available: "<<temp->qtyUndo<<endl;
        temp=temp->next; i++;
    }
}

void undoOrderAction(){
    if(!orderUndoTop){ cout<<"No order actions to undo.\n"; return; }
    showOrderUndoList();

    int choice, qtyAsk;
    cout<<"\nSelect action number to undo: "; cin>>choice;

    OrderUndo* temp=orderUndoTop;
    OrderUndo* prev=NULL; int index=1;
    while(temp && index<choice){ prev=temp; temp=temp->next; index++; }
    if(!temp){ cout<<"Invalid choice.\n"; return; }

    Product* p=searchProduct(proot,temp->pid);
    if(!p){ cout<<"Product not found.\n"; return; }

    cout<<"Enter quantity to undo (Available: "<<temp->qtyUndo<<"): ";
    cin>>qtyAsk;
    if(qtyAsk<=0 || qtyAsk>temp->qtyUndo){ cout<<"Invalid quantity.\n"; return; }

    p->qty += qtyAsk;
    warehouseStock -= qtyAsk;
    cout<<qtyAsk<<" units restored to product "<<p->name<<endl;

    temp->qtyUndo -= qtyAsk;
    if(temp->qtyUndo==0){
        if(prev==NULL) orderUndoTop=temp->next;
        else prev->next=temp->next;
        delete temp;
    }
}

// =================== GRAPH (Hard-coded) ===================
int graph[10][10]; int shelves=8;
void initGraph(){
    for(int i=0;i<10;i++) for(int j=0;j<10;j++) graph[i][j]=0;
    graph[0][1]=graph[1][0]=1;
    graph[1][2]=graph[2][1]=1;
    graph[2][3]=graph[3][2]=1;
    graph[3][4]=graph[4][3]=1;
    graph[4][5]=graph[5][4]=1;
    graph[5][6]=graph[6][5]=1;
    graph[6][7]=graph[7][6]=1;
    graph[0][5]=graph[5][0]=1;
    graph[2][6]=graph[6][2]=1;
}

void showGraph(){
    cout<<"\nWarehouse Graph:\n";
    for(int i=0;i<shelves;i++){
        cout<<"Shelf "<<i<<": ";
        for(int j=0;j<shelves;j++) if(graph[i][j]==1) cout<<j<<" ";
        cout<<endl;
    }
}

// =================== FILE HANDLING ===================
void saveProducts(Product* r, ofstream &f){
    if(!r) return;
    saveProducts(r->left,f);
    f<<r->id<<" "<<r->name<<" "<<r->qty<<" "<<r->shelf<<"\n";
    saveProducts(r->right,f);
}

void loadProducts(){
    ifstream f("products.txt");
    if(!f) return;
    int id, qty, shelf; char name[50];
    while(f>>id>>name>>qty>>shelf) proot=insertProduct(proot,id,name,qty,shelf);
}

void saveAll(){
    ofstream fp("products.txt");
    saveProducts(proot,fp);
    fp.close();
}

// =================== PROCESS ORDER ===================
void processOrder(){
    Order* o=heapRemoveMax();
    if(!o){ cout<<"No orders.\n"; return; }

    Product* p=searchProduct(proot,o->pid);
    if(!p){ cout<<"Product not found.\n"; delete o; return; }

    if(p->qty<o->qty){ cout<<"Insufficient stock.\n"; delete o; return; }

    p->qty -= o->qty;
    p->freq += o->qty;
    recordOrderUndo(p->id,o->qty);

    Picker* pk=getLeastLoadedPicker();
    if(pk){
        cout<<"Assigned picker: "<<pk->name<<" (Pending orders: "<<pk->pendingOrders<<")"<<endl;
        pk->pendingOrders++; pk->busy=true;
    } else cout<<"All pickers busy, order will wait.\n";

    cout<<"\n=== ORDER MANIFEST ===\n";
    cout<<"Customer: "<<o->customerName<<" | Contact: "<<o->customerContact<<endl;
    cout<<"Product: "<<p->name<<" | Qty: "<<o->qty<<" | Shelf: "<<p->shelf<<endl;
    cout<<"Estimated Time: "<<o->qty<<" units\n";
    cout<<"-----------------------\n";

    o->completed=true;
    logFile<<"Processed Order "<<o->oid<<" for "<<o->customerName<<endl;
    delete o;
}

// =================== MAIN ===================
int main(){
    logFile.open("output.txt", ios::app);
    initGraph();
    loadProducts();

    addPicker(1,"Ali"); addPicker(2,"Sara"); addPicker(3,"Nadia");

    int ch;
    while(true){
        cout<<"\n=== SIMPLE WAREHOUSE SYSTEM ===\n";
        cout<<"1. Add Product\n2. Edit Product\n3. Delete Product\n4. Search Product\n5. Prefix Search\n";
        cout<<"6. Add Order\n7. Process Order\n8. Show Graph\n";
        cout<<"9. Undo Product Action\n10. Undo Order Action\n11. Show Products\n0. Exit\n";
        cout<<"Enter: "; cin>>ch;
        if(ch==0) break;

        if(ch==1){
            int id, qty, shelf; char name[50];
            cout<<"ID Name Qty Shelf: "; cin>>id>>name>>qty>>shelf;
            Product* p=searchProduct(proot,id);
            if(p){ p->qty+=qty; cout<<"Quantity added.\n"; }
            else { proot=insertProduct(proot,id,name,qty,shelf); cout<<"Product added.\n"; }
            recordUndo(id,qty); warehouseStock-=qty;
        }
        else if(ch==2){
            int id; cout<<"ID: "; cin>>id;
            Product* p=searchProduct(proot,id);
            if(!p){ cout<<"Not found.\n"; continue; }
            cout<<"New name new qty new shelf: "; cin>>p->name>>p->qty>>p->shelf;
        }
        else if(ch==3){
            int id; cout<<"ID to delete: "; cin>>id;
            proot=deleteProduct(proot,id); cout<<"Product deleted.\n";
        }
        else if(ch==4){
            int id; cout<<"ID: "; cin>>id;
            Product* p=searchProduct(proot,id);
            if(p) cout<<"Found: "<<p->name<<" Qty "<<p->qty<<endl;
            else cout<<"Not found.\n";
        }
        else if(ch==5){
            char prefix[50]; cout<<"Prefix: "; cin>>prefix;
            prefixSearch(proot,prefix);
        }
        else if(ch==6){
            cout<<"\n--- Available Products ---\n";
            if(!proot) cout<<"No products available!\n";
            else displayProducts(proot);
            cout<<"--------------------------\n";

            int n; cout<<"How many orders to add? "; cin>>n;
            for(int i=0;i<n;i++){
                int oid,pid,qty,pr;
                char cname[50], ccontact[50];
                cout<<"Order "<<i+1<<" (OID PID Qty Priority CustomerName CustomerContact): ";
                cin>>oid>>pid>>qty>>pr>>cname>>ccontact;

                Product* p=searchProduct(proot,pid);
                if(!p){ cout<<"Product ID "<<pid<<" does NOT exist.\n"; continue; }
                if(qty>p->qty){ cout<<"Not enough stock! Available: "<<p->qty<<"\n"; continue; }

                Order* o=new Order();
                o->oid=oid; o->pid=pid; o->qty=qty; o->priority=pr;
                strcpy(o->customerName,cname); strcpy(o->customerContact,ccontact);
                o->completed=false;

                heapInsert(o);
                cout<<" Order added successfully.\n";
            }
        }
        else if(ch==7) processOrder();
        else if(ch==8) showGraph();
        else if(ch==9) undoProductAction();
        else if(ch==10) undoOrderAction();
        else if(ch==11) displayProducts(proot);

        completePickerTasks();
    }

    saveAll(); logFile.close();
    return 0;
}
