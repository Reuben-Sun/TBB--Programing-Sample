#include <iostream>
#include <string>
#include <tbb/tbb.h>

struct MyHashCompare{
    static size_t hash(const std::string& s){
        size_t h = 0;
        for(auto &c : s){
            h = (h*17)^c;
        }
        return h;
    }
    static bool equal(const std::string& x, const std::string& y){
        return x == y;
    }
};

typedef tbb::concurrent_hash_map<std::string, int, MyHashCompare> StringTable;

//一个函数对象，用于记录table内元素数量
class Tally{
private:
    StringTable& table;
public:
    Tally(StringTable& table_): table(table_) {}
    void operator() (const tbb::blocked_range<std::string*> range) const {
        for(std::string* p = range.begin(); p != range.end(); ++p){
            StringTable::accessor a;
            table.insert(a, *p);
            a->second += 1;
        }
    }
};

const size_t N = 10;

std::string Data[N] = { "Hello", "World", "TBB", "Hello",
                        "So Long", "Thanks for all the fish", "So Long",
                        "Three", "Three", "Three" };

int main() {
    StringTable table;

    // Put occurrences into the table
    tbb::parallel_for( tbb::blocked_range<std::string*>( Data, Data+N, 1000 ),
                       Tally(table) );

    // Display the occurrences using a simple walk
    // (note: concurrent_hash_map does not offer const_iterator)
    // see a problem with this code???
    // read "Iterating thorough these structures is asking for trouble"
    // coming up in a few pages
    for( StringTable::iterator i=table.begin();
         i!=table.end();
         ++i )
        printf("%s %d\n",i->first.c_str(),i->second);


    return 0;
}
