////////////////////////////////////////////////////////////////////////
// File:       DigiArapucaSBNDAlg.h
//
// This algorithm is used for the electronic response of arapucas
// Created by L. Paulucci and F. Marinho
// Based on OpDetDigitizerDUNE_module.cc and SimPMTSBND_module.cc
////////////////////////////////////////////////////////////////////////

#ifndef SBND_OPDETSIM_DIGIARAPUCASBNDALG_H
#define SBND_OPDETSIM_DIGIARAPUCASBNDALG_H

#include "fhiclcpp/types/Atom.h"
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "nurandom/RandomUtils/NuRandomService.h"
#include "CLHEP/Random/RandomEngine.h"
#include "CLHEP/Random/JamesRandom.h"
#include "CLHEP/Random/RandFlat.h"
#include "CLHEP/Random/RandGaussQ.h"
#include "CLHEP/Random/RandExponential.h"

#include <algorithm>
#include <memory>
#include <vector>
#include <cmath>
#include <string>
#include <map>
#include <unordered_map>
#include <set>
#include <sstream>
#include <fstream>

#include "lardataobj/RawData/OpDetWaveform.h"
#include "lardata/DetectorInfoServices/DetectorClocksServiceStandard.h"
#include "lardataobj/Simulation/SimPhotons.h"
#include "lardata/DetectorInfoServices/LArPropertiesService.h"

#include "TMath.h"
#include "TF1.h"
#include "TFile.h"
#include "TH1D.h"

namespace opdet {

  class DigiArapucaSBNDAlg {

  public:

    struct ConfigurationParameters_t {
      double PeakTime;       // maximum time of each component pulse in ns
      double PulseLength;    // in ns
      double MeanAmplitude;  // Mean amplitude for pulses in mV
      double RiseTime;       // pulse rising time constant (exponential)
      double FallTime;       // pulse decaying time constant (exponential)
      double ADC;            //voltage to ADC convertion scale
      double Baseline;       //waveform baseline
      double BaselineRMS;    //Pedestal RMS in ADC counts
      double DarkNoiseRate;  //in Hz
      double CrossTalk;      //probability for producing a signal of 2 PE in response to 1 photon
      double Saturation;     //Saturation in number of p.e.
      double ArapucaEffT1;   //Arapuca type 1 efficiency (optical window + cavity)
      double ArapucaEffT2;   //Arapuca type 2 efficiency (optical window + cavity)
      double ArapucaEffxT1;    //X-Arapuca T1 efficiency (optical window + cavity)
      double ArapucaEffxT2;    //X-Arapuca T2 efficiency (optical window + cavity)
      std::string ArapucaDataFile; //File containing timing structure for arapucas

      detinfo::LArProperties const* larProp = nullptr; ///< LarProperties service provider.
      detinfo::DetectorClocks const* timeService = nullptr; ///< DetectorClocks service provider.
      CLHEP::HepRandomEngine* engine = nullptr;
    };// ConfigurationParameters_t

    //Default constructor
    DigiArapucaSBNDAlg(ConfigurationParameters_t const& config);
    //Default destructor
    ~DigiArapucaSBNDAlg();

    double Baseline()
    {
      return fParams.Baseline;
    }

    void ConstructWaveform(int ch,
                           sim::SimPhotons const& simphotons,
                           std::vector<short unsigned int>& waveform,
                           std::string pdtype,
                           double start_time,
                           unsigned n_samples);
    void ConstructWaveformLite(int ch,
                               sim::SimPhotonsLite const& litesimphotons,
                               std::vector<short unsigned int>& waveform,
                               std::string pdtype,
                               double start_time,
                               unsigned n_samples);

  private:

    // Declare member data here.
    ConfigurationParameters_t fParams;

    double fSampling;        //wave sampling frequency (GHz)
    int pulsesize;
    double fArapucaEffT1;
    double fArapucaEffT2;
    double fArapucaEffxT1;
    double fArapucaEffxT2;

    double saturation;

    CLHEP::HepRandomEngine* fEngine; //!< Reference to art-managed random-number engine

    TH1D* TimeArapucaT1; //histogram for getting the photon time distribution inside the arapuca T1 box (considering the optical window)
    TH1D* TimeArapucaT2; //histogram for getting the photon time distribution inside the arapuca T2 box (considering the optical window)
    TH1D* TimeArapucaX; //histogram for getting the photon time distribution inside the X-arapuca box (considering the optical window)

    std::vector<double> wsp; //single photon pulse vector
    std::unordered_map< raw::Channel_t, std::vector<double> > fFullWaveforms;

    void CreatePDWaveform(sim::SimPhotons const& SimPhotons,
                          double t_min,
                          std::vector<double>& wave,
                          std::string pdtype);
    void CreatePDWaveformLite(std::map<int, int> const& photonMap,
                              double t_min,
                              std::vector<double>& wave,
                              std::string pdtype);
    void AddSPE(size_t time_bin, std::vector<double>& wave, int nphotons); // add single pulse to auxiliary waveform
    void Pulse1PE(std::vector<double>& wave);
    void AddLineNoise(std::vector<double>& wave);
    void AddDarkNoise(std::vector<double>& wave);
    double FindMinimumTime(sim::SimPhotons const& simphotons);
    double FindMinimumTimeLite(std::map< int, int > const& photonMap);
    void CreateSaturation(std::vector<double>& wave);//Including saturation effects
  };//class DigiArapucaSBNDAlg

  class DigiArapucaSBNDAlgMaker {

  public:
    struct Config {
      using Name = fhicl::Name;
      using Comment = fhicl::Comment;

      fhicl::Atom<double> peakTime {
        Name("ArapucaPeakTime"),
        Comment("Single pe: Time of maximum amplitude in the SiPM pulse in ns")
      };

      fhicl::Atom<double> pulseLength {
        Name("ArapucaPulseLength"),
        Comment("Single pe: Time length of SiPM pulse")
      };

      fhicl::Atom<double> meanAmplitude {
        Name("ArapucaMeanAmplitude"),
        Comment("Single pe: mean amplitude of SiPM pulse in mV")
      };

      fhicl::Atom<double> riseTime {
        Name("ArapucaRiseTime"),
        Comment("Single pe: Pulse rise time constant (exponential), from 0.1 to 0.9 of maximum amplitude")
      };

      fhicl::Atom<double> fallTime {
        Name("ArapucaFallTime"),
        Comment("Single pe: Pulse decay time constant (exponential), from 0.1 to 0.9 of maximum amplitude")
      };

      fhicl::Atom<double> voltageToADC {
        Name("ArapucaVoltageToADC"),
        Comment("Voltage to ADC convertion factor")
      };

      fhicl::Atom<double> baseline {
        Name("ArapucaBaseline"),
        Comment("Waveform baseline in ADC")
      };

      fhicl::Atom<double> baselineRMS {
        Name("ArapucaBaselineRMS"),
        Comment("RMS of the electronics noise fluctuations in ADC counts")
      };

      fhicl::Atom<double> darkNoiseRate {
        Name("ArapucaDarkNoiseRate"),
        Comment("Dark noise rate in Hz")
      };

      fhicl::Atom<double> crossTalk {
        Name("CrossTalk"),
        Comment("Probability for producing a signal of 2 PE in response to 1 photon")
      };

      fhicl::Atom<double> saturation {
        Name("ArapucaSaturation"),
        Comment("Saturation in number of p.e.")
      };

      fhicl::Atom<double> arapucaEffT1 {
        Name("ArapucaEffT1"),
        Comment("Arapuca type 1 efficiency (optical window + cavity)")
      };

      fhicl::Atom<double> arapucaEffT2 {
        Name("ArapucaEffT2"),
        Comment("Arapuca type 2 efficiency (optical window + cavity)")
      };

      fhicl::Atom<double> arapucaEffxT1 {
        Name("ArapucaEffxT1"),
        Comment("X-Arapuca efficiency T1 (optical window + cavity)")
      };

      fhicl::Atom<double> arapucaEffxT2 {
        Name("ArapucaEffxT2"),
        Comment("X-Arapuca efficiency T2 (optical window + cavity)")
      };

      fhicl::Atom<std::string> arapucaDataFile {
        Name("ArapucaDataFile"),
        Comment("File containing timing distribution for Arapuca T1 (optical window + cavity), Arapuca T2 (optical window + cavity), X-Arapuca T1 (optical window)")
      };
    };    //struct Config

    DigiArapucaSBNDAlgMaker(Config const& config); //Constructor

    std::unique_ptr<DigiArapucaSBNDAlg> operator()(
      detinfo::LArProperties const& larProp,
      detinfo::DetectorClocks const& detClocks,
      CLHEP::HepRandomEngine* engine
    ) const;

  private:
    /// Part of the configuration learned from configuration files.
    DigiArapucaSBNDAlg::ConfigurationParameters_t fBaseConfig;
  }; //class DigiArapucaSBNDAlgMaker

} //namespace

#endif //SBND_OPDETSIM_DIGIARAPUCASBNDALG
