////////////////////////////////////////////////////////////////////////
// Class:       VSTChannelMap
// Plugin Type: service (art v2_07_03)
// File:        VSTChannelMap_service.cc
//
// Generated at Mon Jun 18 12:05:37 2018 by Gray Putnam using cetskelgen
// from cetlib version v3_00_01.
////////////////////////////////////////////////////////////////////////

#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <cctype>
#include <stdlib.h>
#include <cassert>

#include "art/Framework/Services/Registry/ActivityRegistry.h"
#include "art/Framework/Services/Registry/ServiceMacros.h"
#include "fhiclcpp/ParameterSet.h"

#include "VSTChannelMap.hh"
#include "HeaderData.hh"

using namespace std;

daqAnalysis::VSTChannelMap::VSTChannelMap(fhicl::ParameterSet const & p, art::ActivityRegistry & areg):
  _n_channels(p.get<unsigned>("n_channels")),
  _n_fem(p.get<unsigned>("n_fem")),
  _slot_offset(p.get<unsigned>("slot_offset", 0)),
  _crate_id(p.get<unsigned>("crate_id", 0)),
  _channel_to_wire(),
  _wire_to_channel(),
  _wire_per_fem(_n_fem, 0)
{
  // Setup channel map

  // read channel map from file if defined
  if (p.has_key("map_file_name")) {
    // in this case, we need to know the number of channel per fem
    _channel_per_fem = p.get<unsigned>("channel_per_fem");
    fstream input(p.get<string>("map_file_name"), fstream::in);
    /* 
     * Data format is (from Jose):
     *
     * (int)TPC_wire (string)plane (string)adapter_connector (int)adapter_pin (int)analog_connector (int)analog_pin (int)ASIC (int)ASIC_ch (int)FEMB_ch (int)FEMB (int)WIB (int)FEM_slot (int)FEM_ch
     *
     * Not all FEM channels are used. Those that are unused have a TPC_wire value of -1.
     * Wire ID's are per plane and FEM channel ID's are per slot
     * Wire ID's are 1-indexed and FEM channel ID's are 0-indexed
     *
     *
     * For analysis code, we want to make the wire ID's and the FEM channel ID's defined globally and make both 0-indexed.
     * So define induction plane = [0, 240) and collection plane = [240, 480).
     * And define global_channel_id = slot_id * 64 + channel_id;
    */
    unsigned channel_count = 0;
    unsigned line_no = 0;
    string line;

    // line fields
    int tpc_wire;
    string wire_plane;
    string adapter_connector;
    int adapter_pin; 
    int analog_connector;
    int analog_pin;
    int ASIC;
    int ASIC_ch;
    int FEMB_ch;
    int FEMB;
    int WIB;
    unsigned FEM_slot;
    unsigned FEM_ch;

    while (getline(input, line)) {
      istringstream sline(line);
      // checks if line is formatted properly
      bool formatted = (sline >> tpc_wire >> wire_plane >> adapter_connector >> adapter_pin >> analog_connector >>
          analog_pin >> ASIC >> ASIC_ch >> FEMB_ch >> FEMB >> WIB >> FEM_slot >> FEM_ch >> std::ws).good();
      if (!formatted && sline.peek() != EOF) {
        cerr << "ERROR: misformatted line " << line_no << endl;
        exit(1);
      }
      line_no ++;
      if (tpc_wire < 0) {
        continue;
      }
      unsigned wire_id;
      if (wire_plane == "Induction") {
        wire_id = (unsigned)tpc_wire - 1;
      }
      else if (wire_plane == "Collection") {
        wire_id = (unsigned)tpc_wire + _n_channels/2 - 1;
      }
      else {
        cerr << "ERROR: bad plane value: " << wire_plane << std::endl;
        exit(1);
      }
      unsigned channel_id = (FEM_slot - _slot_offset) * _channel_per_fem + FEM_ch;

      // set channel map
      _channel_to_wire[channel_id] = wire_id;
      _wire_to_channel[wire_id] = channel_id;
      // incl wire per fem
      _wire_per_fem[FEM_slot - _slot_offset] += 1;

      channel_count ++;
    }
    // checks if file has correct number of lines
    if (channel_count != _n_channels) {
      cerr << "ERROR: incorrect number of channels defined in file " << channel_count << " lines for " << _n_channels << " channels." << std::endl;
      exit(1);
    }
  }
  else {
    for (unsigned i = 0; i < _n_channels; i++) {
      _channel_to_wire[i] = i;
      _wire_to_channel[i] = i;
    }
    // setup channels per fem
    // assume channels are spread equally over FEM
    if (_n_channels % _n_fem != 0) {
      std::cerr << "ERROR: Must have equal number of channels per FEM" << std::endl;
      exit(1);
    }
    _channel_per_fem = _n_channels / _n_fem;
    for (unsigned i = 0; i < _n_fem; i++) {
      _wire_per_fem[i] = _channel_per_fem;
    }
  }
}

unsigned daqAnalysis::VSTChannelMap::NFEM() const { return _n_fem; } 
unsigned daqAnalysis::VSTChannelMap::NChannels() const { return _n_channels; }
unsigned daqAnalysis::VSTChannelMap::Channel2Wire(unsigned i) const { return _channel_to_wire.at(i); }
unsigned daqAnalysis::VSTChannelMap::Wire2Channel(unsigned i) const { return _wire_to_channel.at(i); }
unsigned daqAnalysis::VSTChannelMap::NSlotWire(unsigned i) const { return _wire_per_fem[i]; }
unsigned daqAnalysis::VSTChannelMap::NSlotChannel() const { return _channel_per_fem; }

bool daqAnalysis::VSTChannelMap::IsMappedChannel(unsigned channel_no) const {
  // if the map can't find the key before hitting the end, then the key doesn't exist
  return _channel_to_wire.find(channel_no) != _channel_to_wire.end();
}

unsigned daqAnalysis::VSTChannelMap::Channel2Wire(unsigned channel, unsigned slot, unsigned crate, bool add_offset) const {
  return Channel2Wire(ReadoutChannel2Ind(channel, slot, crate, add_offset));
}

unsigned daqAnalysis::VSTChannelMap::Channel2Wire(daqAnalysis::ReadoutChannel channel) const {
  return Channel2Wire(ReadoutChannel2Ind(channel));
}

bool daqAnalysis::VSTChannelMap::IsMappedChannel(unsigned channel, unsigned slot, unsigned crate, bool add_offset) const {
  return IsMappedChannel(ReadoutChannel2Ind(channel, slot, crate, add_offset));
}

bool daqAnalysis::VSTChannelMap::IsMappedChannel(daqAnalysis::ReadoutChannel channel) const {
  return IsMappedChannel(ReadoutChannel2Ind(channel));
}

daqAnalysis::ReadoutChannel daqAnalysis::VSTChannelMap::Ind2ReadoutChannel(unsigned channel_no) const {
  daqAnalysis::ReadoutChannel ret;
  ret.crate = _crate_id;
  ret.slot = channel_no / _channel_per_fem + _slot_offset;
  ret.channel_ind = channel_no % _channel_per_fem;
  return ret;
}

unsigned daqAnalysis::VSTChannelMap::ReadoutChannel2Ind(unsigned channel, unsigned slot, unsigned crate, bool add_offset) const {
  daqAnalysis::ReadoutChannel readout;
  readout.channel_ind = channel;
  readout.slot = slot + (add_offset ? _slot_offset : 0);
  readout.crate = crate;
  return ReadoutChannel2Ind(readout);
}

unsigned daqAnalysis::VSTChannelMap::ReadoutChannel2Ind(daqAnalysis::ReadoutChannel channel) const {
  return (channel.slot - _slot_offset) * _channel_per_fem + channel.channel_ind; 
}


unsigned daqAnalysis::VSTChannelMap::SlotIndex(daqAnalysis::HeaderData header) const {
  return header.slot - _slot_offset;
}

unsigned daqAnalysis::VSTChannelMap::SlotIndex(daqAnalysis::ReadoutChannel channel) const {
  return channel.slot - _slot_offset;
}

unsigned daqAnalysis::VSTChannelMap::SlotIndex(daqAnalysis::NevisTPCMetaData metadata) const {
  return metadata.slot - _slot_offset;
}


bool daqAnalysis::VSTChannelMap::IsGoodSlot(unsigned slot) const {
  // negative overflow will wrap around and also be large than n_fem_per_crate,
  // so this covers both the case where the slot id is too big and too small
  return (slot - _slot_offset) < _n_fem;
}

DEFINE_ART_SERVICE(daqAnalysis::VSTChannelMap)
