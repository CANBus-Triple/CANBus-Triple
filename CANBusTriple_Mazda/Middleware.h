
#ifndef CANMiddleware_H
#define CANMiddleware_H

class Middleware
{
  public:
   static void init();
   static void tick();
   static Message process( Message msg );
};

Message Middleware::process( Message msg )
{
  return msg;
}


#endif
