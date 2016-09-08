#include <MessageQueue.h>
#include "Middleware.h"


class ChannelSwap : public Middleware
{
  public:
    void tick();
    Message process( Message msg );
    ChannelSwap(){};
    void commandHandler(byte* bytes, int length);
};

void ChannelSwap::tick(){}

void ChannelSwap::commandHandler(byte* bytes, int length){}

Message ChannelSwap::process( Message msg )
{

  switch( msg.busId ){
   case 1:
     msg.busId = 3;
     msg.dispatch = true;
   break;
   case 2:
     msg.dispatch = false;
   break;
   case 3:
     msg.busId = 1;
     msg.dispatch = true;
   break;
  }

  return msg;

}
