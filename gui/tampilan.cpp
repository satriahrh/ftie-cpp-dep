#include "tampilan.h"

#include "../ftie/ftie.h"

#include <iostream>
#include <cstdint>
#include <exception>
#include <string>
#include <sstream>


Tampilan::Tampilan():
frameMethod("Method"),
frameType("Operation Type"),
frameKeystream("Keystream"),
frameBbs("BBS Key"),
frameAcm("ACM Key"),
frameRun("Finish"),
boxV(Gtk::ORIENTATION_VERTICAL),
boxMethod(Gtk::ORIENTATION_HORIZONTAL),
boxType(Gtk::ORIENTATION_HORIZONTAL),
boxKeystream(Gtk::ORIENTATION_HORIZONTAL),
boxBbs(Gtk::ORIENTATION_VERTICAL),
boxAcm(Gtk::ORIENTATION_VERTICAL),
boxAcmA(Gtk::ORIENTATION_HORIZONTAL),
boxBbsP(Gtk::ORIENTATION_HORIZONTAL),
boxBbsQ(Gtk::ORIENTATION_HORIZONTAL),
boxBbsS(Gtk::ORIENTATION_HORIZONTAL),
boxAcmB(Gtk::ORIENTATION_HORIZONTAL),
boxAcmN(Gtk::ORIENTATION_HORIZONTAL),
boxRun(Gtk::ORIENTATION_HORIZONTAL),
buttonRun("RUN"),
cbProposed("Proposed FTIE"),
cbPrevious("Previous FTIE"),
rbEncrypt("Encryption"),
rbDecrypt("Decryption"),
labelBbsP("  p  :  "),
labelBbsQ("  q  :  "),
labelBbsS("  s  :  "),
labelAcmA("  a  :  "),
labelAcmB("  b  :  "),
labelAcmN("  n  :  "),
labelMessage("click \"RUN\" if you are ready")
{
  set_title("File to Image Encryption");

  // ftie::encrypt(6967, 6971, 8514112, 2, 3, 2, "/home/hafizhme/code/ftie-cpp/README.md", "/home/hafizhme/code/ftie-cpp/README.png");
  // ftie::decrypt(6967, 6971, 8514112, 2, 3, 2, "/home/hafizhme/code/ftie-cpp/README.png", "/home/hafizhme/code/ftie-cpp/README.md");
  // TODO check previous method

  add(boxV);

  // METHOD
  boxV.add(frameMethod);
  frameMethod.add(boxMethod);
  boxMethod.pack_start(cbProposed);
  boxMethod.pack_start(cbPrevious);
  cbProposed.set_active();
  cbPrevious.set_active(false);
  cbPrevious.signal_clicked().connect(sigc::mem_fun(*this, &Tampilan::on_cb_previous_clicked));
  cbProposed.signal_clicked().connect(sigc::mem_fun(*this, &Tampilan::on_cb_proposed_clicked));

  // TYPE
  boxV.add(frameType);
  frameType.add(boxType);
  rbDecrypt.join_group(rbEncrypt);
  boxType.pack_start(rbEncrypt);
  boxType.pack_start(rbDecrypt);
  rbEncrypt.set_active();
  rbDecrypt.set_active(false);

  boxV.add(frameKeystream);
  frameKeystream.add(boxKeystream);
  boxKeystream.pack_start(entryKeystream, Gtk::PACK_EXPAND_WIDGET);
  boxKeystream.hide();
  entryKeystream.set_editable(false);
  frameKeystream.set_label("Keystream (disabled)");

  boxV.add(frameBbs);
  frameBbs.add(boxBbs);
  boxBbs.add(boxBbsP);
  boxBbs.add(boxBbsQ);
  boxBbs.add(boxBbsS);
  boxBbsP.pack_start(labelBbsP, Gtk::PACK_SHRINK);
  boxBbsP.pack_start(entryBbsP, Gtk::PACK_SHRINK);
  boxBbsQ.pack_start(labelBbsQ, Gtk::PACK_SHRINK);
  boxBbsQ.pack_start(entryBbsQ, Gtk::PACK_SHRINK);
  boxBbsS.pack_start(labelBbsS, Gtk::PACK_SHRINK);
  boxBbsS.pack_start(entryBbsS, Gtk::PACK_SHRINK);

  boxV.add(frameAcm);
  frameAcm.add(boxAcm);
  boxAcm.add(boxAcmA);
  boxAcm.add(boxAcmB);
  boxAcm.add(boxAcmN);
  boxAcmA.pack_start(labelAcmA, Gtk::PACK_SHRINK);
  boxAcmA.pack_start(entryAcmA, Gtk::PACK_SHRINK);
  boxAcmB.pack_start(labelAcmB, Gtk::PACK_SHRINK);
  boxAcmB.pack_start(entryAcmB, Gtk::PACK_SHRINK);
  boxAcmN.pack_start(labelAcmN, Gtk::PACK_SHRINK);
  boxAcmN.pack_start(entryAcmN, Gtk::PACK_SHRINK);

  boxV.add(boxRun);
  boxRun.pack_start(buttonRun, Gtk::PACK_SHRINK);
  buttonRun.signal_clicked().connect(sigc::mem_fun(*this, &Tampilan::on_button_run_clicked));

  boxV.add(labelMessage);

  show_all_children();
}

Tampilan::~Tampilan()
{
}

void Tampilan::on_cb_proposed_clicked() {
  if (cbProposed.get_active()) {
    frameBbs.set_label("BBS Key");
    entryBbsP.set_editable(true);
    entryBbsQ.set_editable(true);
    entryBbsS.set_editable(true);
  } else {
    frameBbs.set_label("BBS Key (disabled)");
    entryBbsP.set_editable(false);
    entryBbsQ.set_editable(false);
    entryBbsS.set_editable(false);
  }
}

void Tampilan::on_cb_previous_clicked() {
  if (cbPrevious.get_active()) {
    frameKeystream.set_label("Keystream");
    entryKeystream.set_editable(true);
  } else {
    frameKeystream.set_label("Keystream (disabled)");
    entryKeystream.set_editable(false);
  }
}

std::string Tampilan::choose_plainfile() {
  Gtk::FileChooserAction action = rbEncrypt.get_active()? Gtk::FILE_CHOOSER_ACTION_OPEN : Gtk::FILE_CHOOSER_ACTION_SAVE;
  Gtk::FileChooserDialog dialog("Please choose a plainfile", action);
  dialog.set_transient_for(*this);

  //Add response buttons the the dialog:
  dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
  dialog.add_button("_Open", Gtk::RESPONSE_OK);

  //Add filters, so that only certain file types can be selected:
  auto filter_any = Gtk::FileFilter::create();
  filter_any->set_name("Any files");
  filter_any->add_pattern("*");
  dialog.add_filter(filter_any);

  //Show the dialog and wait for a user response:
  int result = dialog.run();

  std::string plainfileFilepath;
  //Handle the response:
  if (result == Gtk::RESPONSE_OK){
    plainfileFilepath = dialog.get_filename();
  }

  return plainfileFilepath;
}

std::string Tampilan::choose_cipherimage() {
  Gtk::FileChooserAction action = rbEncrypt.get_active()? Gtk::FILE_CHOOSER_ACTION_SAVE : Gtk::FILE_CHOOSER_ACTION_OPEN;
  Gtk::FileChooserDialog dialog("Please choose a cipherimage", action);
  dialog.set_transient_for(*this);

  //Add response buttons the the dialog:
  dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
  dialog.add_button("_Open", Gtk::RESPONSE_OK);

  //Add filters, so that only certain file types can be selected:
  auto filter_png = Gtk::FileFilter::create();
  filter_png->set_name("PNG");
  filter_png->add_pattern("*.png");
  dialog.add_filter(filter_png);

  //Show the dialog and wait for a user response:
  int result = dialog.run();

  std::string cipherimageFilepath;
  //Handle the response:
  if (result == Gtk::RESPONSE_OK) {
    cipherimageFilepath = dialog.get_filename();
  }

  return cipherimageFilepath;
}

void Tampilan::on_button_run_clicked() {
  labelMessage.set_text("Processing . . .");
  bool encrypt = rbEncrypt.get_active();
  bool decrypt = rbDecrypt.get_active();
  bool proposed = cbProposed.get_active();
  bool previous = cbPrevious.get_active();
  // plainfileFilepath is defined
  // cipherimageFilepath is defined
  uint16_t p, q, a, b, n;
  uint32_t s;
  std::vector<uint8_t> keystream;
  try {
    if (cbProposed.get_active()) {
      p = std::stoi(entryBbsP.get_text());
      q = std::stoi(entryBbsQ.get_text());
      s = std::stoi(entryBbsS.get_text());
    }

    if (cbPrevious.get_active()) {
      std::string keystreamStr = entryKeystream.get_text();
      keystream = std::vector<uint8_t>(keystreamStr.begin(), keystreamStr.end());
    }

    a = std::stoi(entryAcmA.get_text());
    b = std::stoi(entryAcmB.get_text());
    n = std::stoi(entryAcmN.get_text());

    std::string plainfileFilepath;
    std::string cipherimageFilepath;

    if (encrypt && !decrypt) {
      if (proposed) {
        plainfileFilepath = choose_plainfile();
        cipherimageFilepath = choose_cipherimage();
        ftie::encrypt(p, q, s, a, b, n, plainfileFilepath.c_str(), cipherimageFilepath.c_str());
      }
      if (previous) {
        plainfileFilepath = choose_plainfile();
        cipherimageFilepath = choose_cipherimage();
        ftie::deprecated::encrypt(keystream, a, b, n, plainfileFilepath.c_str(), cipherimageFilepath.c_str());
      }
    }
    else if (!encrypt && decrypt) {
      if (proposed) {
        cipherimageFilepath = choose_cipherimage();
        plainfileFilepath = choose_plainfile();
        ftie::decrypt(p, q, s, a, b, n, cipherimageFilepath.c_str(), plainfileFilepath.c_str());
      }
      if (previous) {
        cipherimageFilepath = choose_cipherimage();
        plainfileFilepath = choose_plainfile();
        ftie::deprecated::decrypt(keystream, a, b, n, cipherimageFilepath.c_str(), plainfileFilepath.c_str());
      }
    }
    // std::cout << p << ' ' << q  << ' ' << s << ' ' << a << ' ' << b << ' ' << n << '\n';
    labelMessage.set_text("Done!!");
  } catch (const std::exception& e) {
    std::cout << e.what() << '\n';
    labelMessage.set_text(e.what());
  }
}
