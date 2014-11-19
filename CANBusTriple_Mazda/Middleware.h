
#ifndef CANMiddleware_H
#define CANMiddleware_H

class Middleware
{
  public:
    virtual void tick();
    virtual Message process( Message msg );
    // boolean enabled;
    Middleware(){};
    ~Middleware(){};
};

Message Middleware::process( Message msg ){}


#endif
