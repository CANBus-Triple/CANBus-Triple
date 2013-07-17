
#include "Middleware.h"


class ChannelSwap : Middleware
{
  public:
   static Message process( Message msg );
};

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
