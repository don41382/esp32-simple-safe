#ifndef BEEPSOUND_H
#define BEEPSOUND_H

class BeepSound {
  private:
    int channel;
  public:
    BeepSound(int channel);
    void reset();
    void single();
    void error();
    void upAndRunning();
    void open();
    void close();
    void goodbye();
    void noAccess();
};

#endif