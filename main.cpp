#include <iostream>
#include <map>
#include <vector>
#include <list>
#include <memory>

using namespace std;

typedef struct{
    shared_ptr<void> pool;
    uint32_t used_elements;
    uint32_t deallocated_elements;
} pool_data;

template <class T, int num>
class PoolAllocator{
private:
    list<pool_data> pools;
    std::size_t pool_size;
    T* cur_pointer = nullptr;

public:
    using value_type = T;
    void AllocateNewPool(){
        //cout << "allocate new pool " << endl;
        pool_data pd{nullptr, 0, 0};
        pools.push_back(pd);
        pools.back().pool.reset(static_cast<T*>(::operator new(pool_size*sizeof(T))));
        cur_pointer = static_cast<T*>(pools.back().pool.get());
    }

    PoolAllocator() noexcept : pool_size(num) {
        //cout << "allocator constructor " << pool_size << endl;
        AllocateNewPool();
    };

    template <class U> PoolAllocator (const PoolAllocator<U, num>&) noexcept {}

    T* allocate (std::size_t n){
        //cout << "request for " << n << " elements" << endl;
        if (pools.back().used_elements + n > num) AllocateNewPool();

        pools.back().used_elements += n;
        return static_cast<T *>(::operator new(n, cur_pointer + pools.back().used_elements - n));
    }

    void deallocate (T* p, std::size_t n) {
        //cout << "deallocate " << n << " elements " << p << endl;
        if (n == 0) return;

        if (n >= pool_size) {
            //cout << "deallocate all" << endl;
            int num_tmp = n - pools.back().used_elements;
            pools.back().pool.reset();
            pools.pop_back();
            cur_pointer = static_cast<T *>(pools.back().pool.get());
            deallocate(p, num_tmp);
        } else {
            pools.back().deallocated_elements += n;
            if (pools.back().used_elements <= pools.back().deallocated_elements) {
                pools.back().pool.reset();
                pools.pop_back();
                //cout << "deallocate " << endl;
                if (pools.size()) {
                    cur_pointer = static_cast<T *>(pools.back().pool.get());
                }
            }
        }
    }

    template<class U>
    struct rebind {
        typedef PoolAllocator<U, num> other;
    };

    using propagate_on_container_copy_assignment = std::true_type;
    using propagate_on_container_move_assignment = std::true_type;
    using propagate_on_container_swap = std::true_type; //UB if std::false_type and a1 != a2;
};

template <class T>
class CNode{
public:
    using value_type = T;
    T data;
    CNode<T>* next;
    CNode() : data(0), next(nullptr) {};
    CNode(const T& value) : data(value), next(nullptr) {};
};

template <class T, class Allocator = allocator<T>>
class MyList{
private:
    CNode<T>* first;
    CNode<T>* last;
    CNode<T>* current;
    size_t size;
    Allocator alloc;

public:
    MyList() : first(nullptr), last(nullptr), current(nullptr), size(0){};

    void Add(const T& value){
        CNode<T>* new_element = (CNode<T>*) allocator_traits<Allocator>::allocate(alloc, 1);
        if (first == nullptr){
            first = new_element;
            last = first;
        } else {
            last->next = new_element;
            last = new_element;
        }
        allocator_traits<Allocator>::construct(alloc, last, value);
        size++;
    }

    void SetFirst(){
        current = first;
    }

    T GetCurrentVal(){
        return current->data;
    }

    auto GetCurrent(){
        return current;
    }

    int Size(){
        return size;
    }

    void Next(){
        current = current->next;
    }

    ~MyList(){
        //cout << "MyList destructor" << endl;
        allocator_traits<Allocator>::deallocate(alloc, (CNode<T> *) last, Size());
    }
};

int main() {
    cout << "allocator with map" << endl;
    map<int, int, std::less<int>, PoolAllocator<pair<const int, int>, 10>> test_map;

    test_map.insert(make_pair(0, 1));
    for (int i = 1; i != 10; i++) {
        static int fact = 1;
        fact *= i;
        test_map.insert(make_pair(i, fact));
    }

    for (auto it = test_map.begin(); it != test_map.end(); it++) {
        cout << it->first << " " << it->second << endl;
    }

    cout << endl << "allocator with my_list" << endl;
    MyList<int, PoolAllocator<CNode<int>, 5>> my_list;
    for(int i=0; i<10; i++)
        my_list.Add(i);


    my_list.SetFirst();
    int size = my_list.Size();
    for(int i=0; i<size; i++){
        cout << my_list.GetCurrentVal() << endl;
        my_list.Next();
    }

    return 0;
}
