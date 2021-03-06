//
// ping_receiver.cc : Ping Receiver Main File
// author           : Fabio Silva
//
// Copyright (C) 2000-2002 by the University of Southern California
// $Id: wcv_receiver.cc,v 1.4 2010/03/08 05:54:49 tom_henderson Exp $
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License,
// version 2, as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
//
// Linking this file statically or dynamically with other modules is making
// a combined work based on this file.  Thus, the terms and conditions of
// the GNU General Public License cover the whole combination.
//
// In addition, as a special exception, the copyright holders of this file
// give you permission to combine this file with free software programs or
// libraries that are released under the GNU LGPL and with code included in
// the standard release of ns-2 under the Apache 2.0 license or under
// otherwise-compatible licenses with advertising requirements (or modified
// versions of such code, with unchanged license).  You may copy and
// distribute such a system following the terms of the GNU GPL for this
// file and the licenses of the other code concerned, provided that you
// include the source code of that other code when and as the GNU GPL
// requires distribution of source code.
//
// Note that people who make modified versions of this file are not
// obligated to grant this special exception for their modified versions;
// it is their choice whether to do so.  The GNU General Public License
// gives permission to release a modified version without this exception;
// this exception also makes it possible to release a modified version
// which carries forward this exception.

#include "wcv_receiver.hh"
#include "wcv.h"
#include <algorithm>
using std::swap;

#ifdef NS_DIFFUSION
static class WCVReceiverAppClass : public TclClass {
public:
    WCVReceiverAppClass() : TclClass("Application/DiffApp/PingReceiver/WCV") {}
    TclObject* create(int , const char*const* ) {
	    return(new WCVReceiverApp());
    }
} class_ping_receiver;

int WCVReceiverApp::command(int argc, const char*const* argv) {
  if (argc == 2) {
    if (strcmp(argv[1], "subscribe") == 0) {
      run();
      return TCL_OK;
    }
   }
  if (argc == 4) {
    if (strcmp(argv[1], "config") == 0) {
      if (strcmp(argv[2], "base") == 0) {
	base = (MobileNode*)Node::get_node_by_address(atoi(argv[3]));
	return TCL_OK;
      }
    }
  }
  return DiffApp::command(argc, argv);
}

#endif // NS_DIFFUSION

void WCVReceiverReceive::recv(NRAttrVec *data, NR::handle my_handle)
{
  app_->recv(data, my_handle);
}

void WCVReceiverApp::recv(NRAttrVec *data, NR::handle h)
{
  /*
  if (get_state() != RECEIVING) {
    DiffPrintWithTime(DEBUG_ALWAYS, "Deaf for now\n");
    return;
  }
  */
  NRSimpleAttribute<int> *counterAttr = NULL;
  NRSimpleAttribute<void *> *timeAttr = NULL;
  NRSimpleAttribute<float> *latitudeAttr = NULL;
  NRSimpleAttribute<float> *longitudeAttr = NULL;
  NRSimpleAttribute<double> *energyAttr = NULL;
  NRSimpleAttribute<int> *idAttr = NULL;
  EventTime *probe_event;
  long delay_seconds;
  long delay_useconds;
  float total_delay;
  struct timeval tmv;
  struct timeval* tmv2;

  GetTime(&tmv);

  latitudeAttr = LatitudeAttr.find(data);
  longitudeAttr = LongitudeAttr.find(data);
  energyAttr = EnergyAttr.find(data);
  counterAttr = AppCounterAttr.find(data);
  timeAttr = TimeAttr.find(data);
  idAttr = IDAttr.find(data);

  float lat, lon, energy;
  int id;
  DiffPrintWithTime(DEBUG_ALWAYS, "Resolve packet\n");
  if (latitudeAttr && longitudeAttr){
	  lat = latitudeAttr->getVal();
	  lon = longitudeAttr->getVal();
	  energy = energyAttr->getVal();
	  id = idAttr->getVal();
	  tmv2 = (struct timeval *) timeAttr->getVal();

	  DiffPrintWithTime(DEBUG_ALWAYS, "Received request (%f, %f, %f, %d, %ld.%06ld)\n", lon ,lat, energy, id, tmv2->tv_sec, tmv2->tv_usec);
	  if ((rear+1)%MODULER == front) {
		  DiffPrintWithTime(DEBUG_ALWAYS, "Request Queue full !\n");
	  } else {
		  bool exist = false;
		  for (unsigned i=front;i!=rear;i=(i+1)%MODULER) {
			  if (req_queue[i].lat==lat && req_queue[i].lon==lon){
				  exist = true;
				  DiffPrintWithTime(DEBUG_ALWAYS, "Request already exists!\n");
				  break;
			  }
		  }
		  if (!exist) {
			  WCVNode* node = static_cast<WCVNode*>(((DiffusionRouting *)dr_)->getNode());
			  DiffPrintWithTime(DEBUG_ALWAYS, "sqrt((%lf - %lf)*(%lf - %lf) + (%lf - %lf)*(%lf - %lf)) > %lf => %d\n", node->X(), lon, node->X(), lon, node->Y(), lat, node->Y(), lat, DD, sqrt((node->X() - lon)*(node->X() - lon) + (node->Y() - lat)*(node->Y() - lat)) > DD + EPS);
			  if (sqrt((node->X() - lon)*(node->X() - lon) + (node->Y() - lat)*(node->Y() - lat)) > DD + EPS) {
				  req_queue[rear].lat = lat;
				  req_queue[rear].lon = lon;
				  req_queue[rear].energy = energy;
				  req_queue[rear].handle = id;
				  rear = (rear + 1) % MODULER;
				  DiffPrintWithTime(DEBUG_ALWAYS, "Append request to Request Queue !\n");
			  }
		  }
	  }
  }else{
	  DiffPrintWithTime(DEBUG_ALWAYS, "Failed to resolve packet\n");
  }
  /*

  if (!counterAttr || !timeAttr){
    DiffPrintWithTime(DEBUG_ALWAYS, "Received a BAD packet !\n");
    PrintAttrs(data);
    return;
  }

  // Calculate latency
  probe_event = (EventTime *) timeAttr->getVal();
  delay_seconds = tmv.tv_sec;
  delay_useconds = tmv.tv_usec;

  if ((delay_seconds < probe_event->seconds_) ||
      ((delay_seconds == probe_event->seconds_) &&
       (delay_useconds < probe_event->useconds_))){
    // Time's not synchronized
    delay_seconds = -1;
    delay_useconds = 0;
    DiffPrintWithTime(DEBUG_ALWAYS, "Error calculating delay !\n");
  }
  else{
    delay_seconds = delay_seconds - probe_event->seconds_;
    if (delay_useconds < probe_event->useconds_){
      delay_seconds--;
      delay_useconds = delay_useconds + 1000000;
    }
    delay_useconds = delay_useconds - probe_event->useconds_;
  }
  total_delay = (float) (1.0 * delay_seconds) + ((float) delay_useconds / 1000000.0);

  // Check if this is the first message received
  if (first_msg_recv_ < 0){
    first_msg_recv_ = counterAttr->getVal();
  }

  // Print output message
  if (last_seq_recv_ >= 0){
    if (counterAttr->getVal() < last_seq_recv_){
      // Multiple sources detected, disabling statistics
      last_seq_recv_ = -1;
      DiffPrintWithTime(DEBUG_ALWAYS, "Node%d: Received data %d, total latency = %f!\n",
		((DiffusionRouting *)dr_)->getNodeId(),
		counterAttr->getVal(), total_delay);
    }
    else{
      last_seq_recv_ = counterAttr->getVal();
      num_msg_recv_++;
      DiffPrintWithTime(DEBUG_ALWAYS, "Node%d: Received data: %d, total latency = %f, %% messages received: %f !\n",
		((DiffusionRouting *)dr_)->getNodeId(),
		last_seq_recv_, total_delay,
		(float) ((num_msg_recv_ * 100.00) /
			 ((last_seq_recv_ - first_msg_recv_) + 1)));
    }
  }
  else{
    DiffPrintWithTime(DEBUG_ALWAYS, "Node%d: Received data %d, total latency = %f !\n",
	      ((DiffusionRouting *)dr_)->getNodeId(),
	      counterAttr->getVal(), total_delay);
  }
  */
}

void WCVReceiverApp::schedule(){
  assert(wcv_state == SCHEDULING);
  WCVNode* node = static_cast<WCVNode*>(((DiffusionRouting *)dr_)->getNode());
  MobileNode* sender = NULL;
  struct request popout;

  // Sort request by energy
  int length = rear >= front? rear-front : rear - front + MODULER;
    for (int i = 0; i < length; i++)
    {
        for (int j = 0; j < length - i - 1; j++)
        {
            if (req_queue[(j+front)%MODULER] > req_queue[(j + 1+front)%MODULER])
            {
                swap(req_queue[(j+front)%MODULER], req_queue[(j + 1+front)%MODULER]);
            }
        }
    }


  bool energy_too_low = false;
  double wcv_energy = node -> energy_model()->energy();
  double dist = node->distance(base);
  double get_back = node->moving_energy(dist);
  double reserve = get_back;
  // Hasn't reached the destination, so don't know the exact energy, but popout.energy

  if (wcv_energy < get_back + reserve) {
	  energy_too_low = true;
  } else {
	    if (front != rear) {
		energy_too_low = true;
		set_state(IDLE);
	    }
	  int new_rear = rear;
	  while (front != rear) {
		  popout = req_queue[front];
		  DiffPrintWithTime(DEBUG_ALWAYS, "Travel to (%lf, %lf)\n", popout.lon, popout.lat);

		  if (wcv_handler)
			  delete wcv_handler;
		  sender = (MobileNode*)Node::get_node_by_address(popout.handle);
		  wcv_handler = new WCVHandler(node, this, sender, popout.energy);

		  double moving = node->moving_energy(node->distance(sender));
		  double get_back = node->moving_energy(sender->distance(base));
		  double reserve = get_back;
		  // Dispite how many energy it requires, just go and checkout it out
		  //double to_charge = sender -> energy_model() -> initialenergy() - popout.energy;
		  //double charging_time = to_charge / CHARGING_SPEED;
		  //double idle = node -> idle_energy(charging_time);

		  if (wcv_energy < moving + get_back + reserve ||
				  node->set_destination(popout.lon+DX, popout.lat+DY, MOVING_SPEED, wcv_handler)) {
		      DiffPrintWithTime(DEBUG_ALWAYS, "Failed to set_destination, drop and wait for retry\n");
		      if (wcv_energy < moving + get_back + reserve) {
		            // Failing because other reasons
		            req_queue[new_rear] = req_queue[front];
		            front = (front + 1) % MODULER;
		            new_rear = (new_rear + 1) % MODULER;
		      } else {
		            // failing because energy is too low 
		            // Drop this request
		            front = (front + 1) % MODULER;
		            energy_too_low = false;
		            set_state(IDLE);
		      }
		  } else {
		    set_state(MOVING);
		    return;
		  }
	  }
	  rear = new_rear;
  }

  if (energy_too_low){
	  assert(wcv_energy >= get_back + reserve);
	  set_state(BACK);
	  // Don't need to refill parameters for Handler, so use the existent wcv_handler
	  int ret = node -> set_destination(base->X()+DX, base->Y()+DY, MOVING_SPEED, wcv_handler);
	  assert(ret == 0);
	  // Moving energy
	  node -> energy_model() -> DecrIdleEnergy(dist/MOVING_SPEED, WCV_POWER_PER_M*MOVING_SPEED);
  } else { // If no valid request, then resched
	  if (subHandle_) {
	     DiffPrintWithTime(DEBUG_ALWAYS, "Receiver unsubscribe %u\n", subHandle_);
	     dr_ -> unsubscribe(subHandle_);
	  }
	  set_state(RECEIVING);
	  subHandle_ = setupSubscription();
	  // Don't need to refill parameters for Handler, so use the existent wcv_handler
	  node -> giveup_sched(wcv_handler);
  }
}

void WCVReceiverApp::ack() {
  front = (front + 1) % MODULER;
}

handle WCVReceiverApp::setupSubscription()
{
  NRAttrVec attrs;

  attrs.push_back(NRClassAttr.make(NRAttribute::IS, NRAttribute::INTEREST_CLASS));
  attrs.push_back(NRAlgorithmAttr.make(NRAttribute::IS, NRAttribute::ONE_PHASE_PULL_ALGORITHM));
  attrs.push_back(NRTypeAttr.make(NRAttribute::IS, WCV_TYPE));
  attrs.push_back(EnergyAttr.make(NRAttribute::LT, 0.8));

  handle h = dr_->subscribe(&attrs, mr_);

  ClearAttrs(&attrs);

  return h;
}

void WCVReceiverApp::run()
{
  // Other states indicates WCV is still running
  if (get_state() == CHARGED) {
          if (subHandle_) {
             DiffPrintWithTime(DEBUG_ALWAYS, "Receiver unsubscribe %u\n", subHandle_);
             dr_ -> unsubscribe(subHandle_);
          }
	  set_state(RECEIVING);
	  subHandle_ = setupSubscription();
	  WCVNode* node = static_cast<WCVNode*>(((DiffusionRouting *)dr_)->getNode());
	  if (!wcv_handler)
		  wcv_handler = new WCVHandler(node, this, NULL, 0);
	  node -> recv(wcv_handler);
  }
}

#ifdef NS_DIFFUSION
WCVReceiverApp::WCVReceiverApp()
#else
WCVReceiverApp::WCVReceiverApp(int argc, char **argv)
#endif // NS_DIFFUSION
{
  set_state(CHARGED);
  front = 0;
  rear = 0;
  
  global_debug_level=5;

  last_seq_recv_ = 0;
  num_msg_recv_ = 0;
  first_msg_recv_ = -1;
  base = NULL;

  mr_ = new WCVReceiverReceive(this);

#ifndef NS_DIFFUSION
  parseCommandLine(argc, argv);
  dr_ = NR::createNR(diffusion_port_);
#endif // !NS_DIFFUSION
}

#ifndef NS_DIFFUSION
int main(int argc, char **argv)
{
  WCVReceiverApp *app;

  app = new WCVReceiverApp(argc, argv);
  app->run();

  return 0;
}
#endif // !NS_DIFFUSION
