// g++ mongo.cc -pthread -lmongoclient -lboost_thread -lboost_system -lboost_regex -I $MONGO_DRIVER/include -L $MONGO_DRIVER/lib

#include <cstdlib>
#include <iostream>
#include "mongo/bson/bson.h"
#include "mongo/client/dbclient.h"

using namespace mongo;

void run() {
  mongo::DBClientConnection c;
  c.connect("localhost");

  bob b;
  bo p = b.genOID().append("name", "Jeo").append("age", 33).obj();
  
  c.insert("tutorial.persons", p);

  if (c.getLastError().empty()) {
    std::cout << "insert sucess" << std::endl;
  }

  std::cout << "count:" << c.count("tutorial.persons") << std::endl;
  std::auto_ptr<DBClientCursor> cursor = c.query("tutorial.persons", bo());
  while(cursor->more())
    std::cout << cursor->next().toString() << std::endl;
}

int main()
{
  mongo::client::initialize();
  try {
    run();
    std::cout << "connect ok" << std::endl;
  } catch (const mongo::DBException& e) {
    std::cout << "caught " << e.what() << std::endl;
  }
  return EXIT_SUCCESS;
}
