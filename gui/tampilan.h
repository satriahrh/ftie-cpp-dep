#ifndef GTKMM_TAMPILAN_H
#define GTKMM_TAMPILAN_H

#include <gtkmm.h>
#include <string>

class Tampilan : public Gtk::Window
{
public:
  Tampilan();
  virtual ~Tampilan();

protected:
  //Signal handlers:
  void on_cb_proposed_clicked();
  void on_cb_previous_clicked();
  void on_button_run_clicked();
  std::string choose_plainfile();
  std::string choose_cipherimage();

  //Child widgets:
  Gtk::Frame
    frameMethod,
    frameType,
    frameKeystream, frameBbs, frameAcm,
    frameRun;
  Gtk::Box
    boxV,
    boxMethod,
    boxType,
    boxKeystream, boxBbs, boxAcm,
    boxBbsP, boxBbsQ, boxBbsS,
    boxAcmA, boxAcmB, boxAcmN,
    boxRun;
  Gtk::CheckButton
    cbPrevious, cbProposed;
  Gtk::RadioButton
    rbEncrypt, rbDecrypt;
  Gtk::Entry
    entryKeystream,
    entryBbsP, entryBbsQ, entryBbsS,
    entryAcmA, entryAcmB, entryAcmN;
  Gtk::Button
    buttonRun;
  Gtk::Label
    labelBbsP, labelBbsQ, labelBbsS,
    labelAcmA, labelAcmB, labelAcmN,
    labelMessage;
};

#endif // GTKMM_TAMPILAN_H
