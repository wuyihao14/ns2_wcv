//
// ping_sender.cc : Ping Server Main File
// author         : Fabio Silva
//
// Copyright (C) 2000-2002 by the University of Southern California
// $Id: wcv_sender.cc,v 1.5 2011/10/02 22:32:34 tom_henderson Exp $
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

#include <cmath>
#include "wcv_sender.hh"
#include "diffusion3/filters/diffusion/one_phase_pull.hh"
#include "wcv.h"
#include <unistd.h>
#include <wireless-phy.h>

#ifdef NS_DIFFUSION
static class WCVSenderAppClass : public TclClass {
public:
  WCVSenderAppClass() : TclClass("Application/DiffApp/PingSender/WCV") {}
  TclObject* create(int , const char*const*) {
    return(new WCVSenderApp());
  }
} class_ping_sender;

void WCVSendDataTimer::expire(Event *) {
  a_->send();
}

void WCVSenderApp::send()
{
  struct timeval tmv;

  MobileNode *node = ((DiffusionRouting *)dr_)->getNode();
  node->log_energy(1, true);

  // Send data if we have active subscriptions
  if (num_subscriptions_ > 0){
    // Update time in the packet
    GetTime(&tmv);
    lastEventTime_->seconds_ = tmv.tv_sec;
    lastEventTime_->useconds_ = tmv.tv_usec;

    // Send data probe
    DiffPrintWithTime(DEBUG_ALWAYS, "Node%d: Sending WCV Data %d\n", ((DiffusionRouting *)dr_)->getNodeId(), last_seq_sent_);
    dr_->send(pubHandle_, &data_attr_);

    // Update counter
    last_seq_sent_++;
    counterAttr_->setVal(last_seq_sent_);
  } else {
  // re-schedule the timer 
    sdt_.resched(SEND_DATA_INTERVAL);
  }
}

int WCVSenderApp::command(int argc, const char*const* argv) {
  if (argc == 2) {
    if (strcmp(argv[1], "publish") == 0) {
benign:
	int O = statistics[((DiffusionRouting*)dr_)->getNodeId()];
	DiffPrintWithTime(DEBUG_ALWAYS, "Node %d : Flow %d\n", ((DiffusionRouting*)dr_)->getNodeId(), O);
      run();
      return TCL_OK;
    }
  } else if (argc == 3) {
      if (strcmp(argv[1], "publish") == 0) {
		static map<int, OPPPingSenderApp*>::iterator it;
	      it = WCVNode::node2app.find(((DiffusionRouting*)dr_)->getNodeId());
	      if (it == WCVNode::node2app.end() || !it->second->malicious) {
		      goto benign;
	      }
	      if (strcmp(argv[2], "auto") == 0) {
		  double coefficient = auto_fake_coefficient();
		  if (coefficient < 0) {
			  goto benign;
		  }
		  //run((0.2+coefficient*0.8) * ((DiffusionRouting*)dr_)->getNode()->energy_model()->initialenergy());
		  run(coefficient * ((DiffusionRouting*)dr_)->getNode()->energy_model()->initialenergy());
		  return TCL_OK;
	      } else if (strcmp(argv[2], "accurate") == 0){
		      auto_accurate_fake_coefficient();
		  double coefficient = auto_fake_coefficient();
		  run(coefficient * ((DiffusionRouting*)dr_)->getNode()->energy_model()->initialenergy());
		  return TCL_OK;
	      } else {
		  run(atof(argv[2]));
		  return TCL_OK;
	      }
      }
  }
  if (argc == 4) {
        if (strcmp(argv[1], "config") == 0) {
  	      if (strcmp(argv[2], "rangex") == 0) {
  		      rangex = atof(argv[3]);
  	      }else if (strcmp(argv[2], "rangey") == 0) {
  		      rangey = atof(argv[3]);
  	      }else if (strcmp(argv[2], "n") == 0) {
  		      n = atoi(argv[3]);
  	      }
  	      return TCL_OK;
        }
  }
  return DiffApp::command(argc, argv);
}
#endif // NS_DIFFUSION

void WCVSenderReceive::recv(NRAttrVec *data, NR::handle my_handle)
{
  app_->recv(data, my_handle);
}

void WCVSenderApp::recv(NRAttrVec *data, NR::handle )
{
  NRSimpleAttribute<int> *nrclass = NULL;
  NRSimpleAttribute<int> *nralgorithm = NULL;

  nrclass = NRClassAttr.find(data);
  nralgorithm = NRAlgorithmAttr.find(data);

  if (!nrclass){
    DiffPrintWithTime(DEBUG_ALWAYS, "Received a BAD packet !\n");
    return;
  }

  switch (nrclass->getVal()){

  case NRAttribute::INTEREST_CLASS:

    DiffPrintWithTime(DEBUG_ALWAYS, "Node%d: Received Interest message ! With algorithm: %d\n",((DiffusionRouting*)dr_)->getNodeId() ,nralgorithm->getVal());
    num_subscriptions_++;
    break;

  case NRAttribute::DISINTEREST_CLASS:

    DiffPrintWithTime(DEBUG_ALWAYS, "Received a Disinterest message !\n");
    num_subscriptions_--;
    break;

  default:

    DiffPrintWithTime(DEBUG_ALWAYS, "Received an unknown message (%d)!\n", nrclass->getVal());
    break;

  }

  /*
  if (nralgorithm->getVal() == INTER_ALGORITHM) { 

  }
  */

}

/*
handle WCVSenderApp::setupInterSubscription() {
  NRAttrVec attrs;
  MobileNode *node = ((DiffusionRouting *)dr_)->getNode();

  if (!evil) {
    return NULL;
  }

  attrs.push_back(NRClassAttr.make(NRAttribute::NE, NRAttribute::DATA_CLASS));
  attrs.push_back(NRAlgorithmAttr.make(NRAttribute::IS, INTER_ALGORITHM));
  attrs.push_back(NRScopeAttr.make(NRAttribute::IS, NRAttribute::NODE_LOCAL_SCOPE));
}
*/

handle WCVSenderApp::setupSubscription(double fake)
{
  NRAttrVec attrs;
  MobileNode *node = ((DiffusionRouting *)dr_)->getNode();

  attrs.push_back(NRClassAttr.make(NRAttribute::NE, NRAttribute::DATA_CLASS));
  attrs.push_back(NRAlgorithmAttr.make(NRAttribute::IS, NRAttribute::ONE_PHASE_PULL_ALGORITHM));
  attrs.push_back(NRTypeAttr.make(NRAttribute::IS, WCV_TYPE));
  attrs.push_back(NRScopeAttr.make(NRAttribute::IS, NRAttribute::NODE_LOCAL_SCOPE));
  if (fake < 0) {
	  node -> log_energy(1, true);

	  if (node -> energy_model() -> energy() >
			node -> energy_model() -> initialenergy() * 0.8) {
		  return NULL;
	  }
	  attrs.push_back(EnergyAttr.make(NRAttribute::IS, node->energy_model()->energy()));
  } else {
	  attrs.push_back(EnergyAttr.make(NRAttribute::IS, fake));
  }


  handle h = dr_->subscribe(&attrs, mr_);

  ClearAttrs(&attrs);

  return h;
}

handle WCVSenderApp::setupPublication(double fake)
{
  NRAttrVec attrs;

  attrs.push_back(NRClassAttr.make(NRAttribute::IS, NRAttribute::DATA_CLASS));
  attrs.push_back(NRAlgorithmAttr.make(NRAttribute::IS, NRAttribute::ONE_PHASE_PULL_ALGORITHM));
  attrs.push_back(NRTypeAttr.make(NRAttribute::IS, WCV_TYPE));

  double latitude, longitude, z;

  MobileNode *node = ((DiffusionRouting *)dr_)->getNode();
  node->getLoc(&longitude, &latitude, &z);

  latitudeAttr_ = LatitudeAttr.make(NRAttribute::IS, latitude);
  longitudeAttr_ = LongitudeAttr.make(NRAttribute::IS, longitude);

  attrs.push_back(LatitudeAttr.make(NRAttribute::IS, latitude));
  attrs.push_back(LongitudeAttr.make(NRAttribute::IS, longitude));
  if (fake < 0) {
	  node -> log_energy(1, true);

	  if (node -> energy_model() -> energy() >
			node -> energy_model() -> initialenergy() * 0.8) {
		  return NULL;
	  }
	  attrs.push_back(EnergyAttr.make(NRAttribute::IS, node->energy_model()->energy()));
	  DiffPrintWithTime(DEBUG_ALWAYS, "Node %d publish energy *%x=%lf\n", node->nodeid(), node->energy_model(), node->energy_model()->energy());
  } else {
	  attrs.push_back(EnergyAttr.make(NRAttribute::IS, fake));
	  DiffPrintWithTime(DEBUG_ALWAYS, "Node %d publish fake energy %lf\n", node->nodeid(), fake);
  }
  attrs.push_back(IDAttr.make(NRAttribute::IS, node->nodeid()));
  struct timeval tmv;
  GetTime(&tmv);
  timeAttr_ = TimeAttr.make(NRAttribute::IS, (void *) &tmv, sizeof(tmv));
  attrs.push_back(timeAttr_);


  handle h = dr_->publish(&attrs);

  ClearAttrs(&attrs);

  return h;
}

void WCVSenderApp::run() {
	run(-1);
}

double WCVSenderApp::getDegree(DiffusionRouting* dr, bool out, set<int>& visibles) {
	double degree = 0.0;
	list<FilterEntry*> filterlist = ((DiffusionRouting*)dr)->filterList();
	list<FilterEntry*>::iterator fei = filterlist.begin();
	/*
	 * Has only One Phase Pull Filter
	 */
	list<RoutingEntry*> routinglist = ((OnePhasePullFilterReceive*)((*fei)->cb_))->filter_->routingList();
	list<RoutingEntry*>::iterator rei = routinglist.begin();
	bool sole_routing_flag = false;
	for (;rei!=routinglist.end();rei++) {
		/*
		const static NRSimpleAttribute<int>* classAttr = 
		const static NRSimpleAttribute<int>* algorithmAttr = 
		*/
		static NRAttrVec attrs {
			NRClassAttr.make(NRAttribute::IS, NRAttribute::INTEREST_CLASS), 
			NRAlgorithmAttr.make(NRAttribute::IS, NRAttribute::ONE_PHASE_PULL_ALGORITHM),
			NRTypeAttr.make(NRAttribute::IS, SENSOR_TYPE)
		};
		// If wcv data packet is found ( not charging packet )
		//if (OneWayMatch(&attrs, (*rei)->attrs_) && OneWayMatch((*rei)->attrs_, &attrs)) {
		if (OneWayPerfectMatch(&attrs, (*rei)->attrs_)) {
			assert(sole_routing_flag == false);
			sole_routing_flag = true;
			list<RoundIdEntry*> roundlist = (*rei)->round_ids_;
			list<RoundIdEntry*>::iterator rdi = roundlist.begin();
			/*
			 * no need to find some round_id_entry 
			 */
			for (;rdi!=roundlist.end();rdi++ ){
				if (out) {
					list<OPPGradientEntry*> gl = (*rdi)->gradients_;
					// So degree is no more than N
					degree += 1.0 * gl.size() / roundlist.size();
					for (list<OPPGradientEntry*>::iterator it=gl.begin();it!=gl.end();it++) {
						visibles.insert((*it) -> node_id_);
					}
				} else {
					int node_id = ((DiffusionRouting*)dr_) -> getNodeId();

					list<OPPGradientEntry*> gl = (*rdi)->gradients_;
					// So degree is no more than N, either
					for (list<OPPGradientEntry*>::iterator it=gl.begin();it!=gl.end();it++) {
						OPPGradientEntry* ge = *it;
						visibles.insert(dr->getNodeId());
						if (ge->node_id_ == node_id) {
							degree += 1.0 / roundlist.size();
							break;
						}
					}
				}
			}
			break;
		}
	}

	return degree;
}

int getFlow(DiffusionRouting* dr) {
	int flow = 0;
	list<FilterEntry*> filterlist = ((DiffusionRouting*)dr)->filterList();
	list<FilterEntry*>::iterator fei = filterlist.begin();
	/*
	 * Has only One Phase Pull Filter
	 */
	list<RoutingEntry*> routinglist = ((OnePhasePullFilterReceive*)((*fei)->cb_))->filter_->routingList();
	list<RoutingEntry*>::iterator rei = routinglist.begin();
	bool sole_routing_flag = false;
	for (;rei!=routinglist.end();rei++) {
		/*
		const static NRSimpleAttribute<int>* classAttr = 
		const static NRSimpleAttribute<int>* algorithmAttr = 
		*/
		static NRAttrVec attrs {
			NRClassAttr.make(NRAttribute::IS, NRAttribute::INTEREST_CLASS), 
			NRAlgorithmAttr.make(NRAttribute::IS, NRAttribute::ONE_PHASE_PULL_ALGORITHM),
			NRTypeAttr.make(NRAttribute::IS, SENSOR_TYPE)
		};
		// If wcv data packet is found ( not charging packet )
		if (OneWayPerfectMatch(&attrs, (*rei)->attrs_)) {
			assert(sole_routing_flag == false);
			sole_routing_flag = true;
			flow += (*rei) -> count;
		}
	}
	return flow;
}

double WCVSenderApp::auto_accurate_fake_coefficient() {
	//int O = getFlow((DiffusionRouting*)dr_);
	int O = statistics[((DiffusionRouting*)dr_)->getNodeId()];
	DiffPrintWithTime(DEBUG_ALWAYS, "Node %d : Flow %d\n", ((DiffusionRouting*)dr_)->getNodeId(), O);
}

double WCVSenderApp::expected_neighbors() {

	const static double pi = 3.141592653589793;
	MobileNode* node = ((DiffusionRouting*)dr_)->getNode();
	assert(node->ifhead() != NULL && node->ifhead().next_node() == NULL);
	WirelessPhy* phy = static_cast<WirelessPhy*>(node->ifhead().lh_first);
	double r = static_cast<WirelessChannel*>(phy->channel())->getdistCST() + 5;
        double p = ((pow(r,4)*(3*pi + 11))/3 + 2*(rangey - 2*r)*(pi*pow(r,3) - (2*pow(r,3))/3) + 4*pow(r,4)*(pi - 4/3) - 2*(2*r - rangex)*(pi*pow(r,3) - (2*pow(r,3))/3) - pow(r,2)*pi*(rangey - 2*r)*(2*r - rangex))/pow(rangey*rangex,2);
	double exp = (n-1) * p + 1;
	return exp;
}

double WCVSenderApp::auto_fake_coefficient() {
	set<int> visibles;
	double O = getDegree((DiffusionRouting*)dr_, true, visibles);
	// At lease comes from this node
	double I = 1;
	for (map<int, OPPPingSenderApp*>::iterator it=WCVNode::node2app.begin();
			it!=WCVNode::node2app.end();it++){

		//visibles.insert(it -> first);
		I += getDegree((DiffusionRouting*)(it -> second -> dr()), false, visibles);
	}
	// O: filter1..routing_entry_1..round_id_s..gradient_s
	// I: node_s..filter1..routing_entry_1..round_id_s..gradient_s
	
	// O <= N, I <= N
	

	/*
	int N = visibles.size();
	
	if (N == 0) {
		DiffPrintWithTime(DEBUG_ALWAYS, "Node %d : Neighbors empty !!!\n", ((DiffusionRouting*)dr_)->getNodeId());
		return -1;
	}
	*/

	double coefficient = 2.0 * (O<I?O:I) / expected_neighbors();

	//assert(coefficient <= 1);

	coefficient = coefficient > 1 ? 1 : coefficient;
	
	int flow = statistics[((DiffusionRouting*)dr_)->getNodeId()];
	DiffPrintWithTime(DEBUG_ALWAYS, "Node %d : Flow %d\n", ((DiffusionRouting*)dr_)->getNodeId(), flow);

	return coefficient;
}

void WCVSenderApp::run(double fake)
{
  struct timeval tmv;
  int retval;

#ifdef INTERACTIVE
  char input;
  fd_set FDS;
#endif // INTERATIVE
  
  // Setup publication and subscription
  if (subHandle_) {
          DiffPrintWithTime(DEBUG_ALWAYS, "Sender unsubscribe %u\n", subHandle_);
	  dr_ -> unsubscribe(subHandle_);
  }
  if (pubHandle_) {
          DiffPrintWithTime(DEBUG_ALWAYS, "Sender unpublish %u\n", pubHandle_);
	  dr_ -> unpublish(pubHandle_);
  }
  subHandle_ = setupSubscription(fake);
  pubHandle_ = setupPublication(fake);

  // Create time attribute
  GetTime(&tmv);
  lastEventTime_ = new EventTime;
  lastEventTime_->seconds_ = tmv.tv_sec;
  lastEventTime_->useconds_ = tmv.tv_usec;
  timeAttr_ = TimeAttr.make(NRAttribute::IS, (void *) &lastEventTime_,
			    sizeof(EventTime));
  data_attr_.push_back(timeAttr_);

  // Change pointer to point to attribute's payload
  delete lastEventTime_;
  lastEventTime_ = (EventTime *) timeAttr_->getVal();

  // Create counter attribute
  counterAttr_ = AppCounterAttr.make(NRAttribute::IS, last_seq_sent_);
  data_attr_.push_back(counterAttr_);

  send();
}

WCVSenderApp::WCVSenderApp() : sdt_(this)
{
  global_debug_level=5;
  last_seq_sent_ = 0;
  num_subscriptions_ = 0;
  mr_ = new WCVSenderReceive(this);
}

