# ======================================================================
# Define options
# ======================================================================
set val(chan)           Channel/WirelessChannel    ;# Channel Type
set val(prop)           Propagation/FreeSpace		;# radio-propagation model
set val(netif)          Phy/WirelessPhy/802_15_4
set val(mac)            Mac/802_15_4
set val(ifq)            Queue/DropTail/PriQueue    ;# interface queue type
set val(ll)             LL                         ;# link layer type
set val(ant)            Antenna/OmniAntenna        ;# antenna model
set opt(filters)        OnePhasePullFilter
set val(ifqlen)         150                         ;# max packet in ifq
set val(rp)             Directed_Diffusion		;# routing protocol

source ./initialize

#Queue/DropTail/PriQueue set Prefer_Routing_Protocols    1
#LL set mindelay_                50us
#LL set delay_                   25us
#LL set bandwidth_               0       ;# not used
#LL set-limit 1000000




# ================================= 
# Antenna Settings 
# ================================= 
Antenna/OmniAntenna set X_ 0 
Antenna/OmniAntenna set Y_ 0 
Antenna/OmniAntenna set Z_ 1.5 
Antenna/OmniAntenna set Gt_ 1.0  
Antenna/OmniAntenna set Gr_ 1.0 
#======================
#Physica layer setting 
#======================
Phy/WirelessPhy set freq_ 2.4e+9 ;# The working band is 2.4GHz 
Phy/WirelessPhy set L_ 0.5   ;#Define the system loss in TwoRayGround 
Phy/WirelessPhy set  bandwidth_  28.8*10e3   ;#28.8 kbps 
# For model 'TwoRayGround'
# The received signals strenghts  in different distances 
set dist(5m)  7.69113e-06
set dist(9m)  2.37381e-06
set dist(10m) 1.92278e-06
set dist(11m) 1.58908e-06
set dist(12m) 1.33527e-06
set dist(13m) 1.13774e-06
set dist(14m) 9.81011e-07
set dist(15m) 8.54570e-07
set dist(16m) 7.51087e-07
set dist(20m) 4.80696e-07
set dist(25m) 3.07645e-07
set dist(30m) 2.13643e-07
set dist(35m) 1.56962e-07
set dist(40m) 1.20174e-07
set dist(100m) 1.92278e-08
Phy/WirelessPhy set CSThresh_  $dist(40m)
Phy/WirelessPhy set RXThresh_  $dist(40m)
#Phy/WirelessPhy set CPThresh_ 10 
# According to the Two-Ray Ground propagation model 
#Pr(d) = Pt*Gt*Gr*ht*ht*hr*hr/(d*d*d*d*L)
# thus the transmitted signal power
#average Pt= d*d*d*d*L*Pr(d)/(Gt*Gr*ht*ht*hr*hr) = 1mW     
# Computed by the propagation/threshold program
Phy/WirelessPhy set  pt_ 0.281838
#Specified Parameters for 802.15.4 MAC
Mac/802_15_4 wpanCmd verbose on   ;# to work in verbose mode 
Mac/802_15_4 wpanNam namStatus on  ;# default = off (should be turned on before other 'wpanNam' commands can work)

# Initialize Global Variables
set ns_  [new Simulator]
# Tell teh simulator to use teh new type trace data 
$ns_ use-newtrace
$ns_ set WirelessNewTrace_ ON
set scenario1   [open scenario1.tr w]
$ns_ trace-all $scenario1
#Define the NAM output file
set scenario1nam     [open scenario1.nam w]
$ns_ namtrace-all-wireless $scenario1nam $val(x) $val(y)
#$ns_ puts-nam-traceall {# nam4wpan #}  ;# inform nam that this is a trace file for wpan (special handling needed)

# set up topography object
set topo       [new Topography]
$topo load_flatgrid $val(x) $val(y)

# Create God 
set god_ [create-god $val(nn)]
set chan_1_ [new $val(chan)]

# configure node
$ns_ node-config -adhocRouting $val(rp) \
  -llType $val(ll) \
  -macType $val(mac) \
  -ifqType $val(ifq) \
  -ifqLen $val(ifqlen) \
  -antType $val(ant) \
  -propType $val(prop) \
  -phyType $val(netif) \
  -topoInstance $topo \
  -diffusionFilter $opt(filters) \
  -agentTrace ON \
  -routerTrace ON \
  -macTrace ON \
  #-movementTrace OFF \
                -energyModel "EnergyModel"\
                -initialEnergy 1\
                -rxPower 0.4\
                -txPower 0.5\
		-idlePower 0.0030\
  -channel $chan_1_ 

for {set i 0} {$i < $val(nn) } {incr i} {
 set node_($i) [$ns_ node $i] 
 $node_($i) random-motion 0  ;# disable random motion
 $god_ new_node $node_($i)
 $node_($i) setenergy 1
}
$node_(3) setenergy 10
$node_(4) setenergy 10
# Define the nodes positions 
source ./location
#set null [new Agent/Null]
#$ns_ attach-agent $node_(3) $null

## Establish traffic between nodes 
   Mac/802_15_4 wpanCmd ack4data off
   puts [format "Acknowledgement for data: %s" [Mac/802_15_4 wpanCmd ack4data]]
#src dest interval start stop packetsize stage 
#the first sensor activity
   
for {set i 0} {$i < 3} {incr i} {
	# src_ is used to communicate with WCV
	set src_($i) [new Application/DiffApp/PingSender/WCV]
	$ns_ attach-diffapp $node_($i) $src_($i)
	# con_ is energy consumer
	set con_($i) [new Application/DiffApp/PingSender/OPP]
	$ns_ attach-diffapp $node_($i) $con_($i)
}
# wcv
set wcv_ [new Application/DiffApp/PingReceiver/WCV]
set snk_ [new Application/DiffApp/PingReceiver/OPP]

$node_(4) NodeLabel wcv
$ns_ attach-diffapp $node_(4) $wcv_
$node_(3) NodeLabel base
$ns_ attach-diffapp $node_(3) $snk_
#set debug_ [gets stdin]

source ./data_flow
source ./charging_request


# defines the node size in nam
for {set i 0} {$i < $val(nn)} {incr i} {
 $ns_ initial_node_pos $node_($i) 5
}

# Tell nodes simulation ends at 30.0
for {set i 0} {$i < $val(nn) } {incr i} {
   $ns_ at 200.000001 "$node_($i) reset";
}
$ns_ at 200 "stop"
$ns_ at 200.000001 "puts \"\nNS EXITING...\""
$ns_ at 200.000001 "$ns_ halt"

proc stop {} {
    global ns_ scenario1 scenario1nam
    $ns_ flush-trace
    close $scenario1
    #set hasDISPLAY 0
    #exec nam scenario1.nam &
 
}
puts "\nStarting Simulation..."
$ns_ run
