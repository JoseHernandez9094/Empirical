//  This file is part of Empirical, https://github.com/devosoft/Empirical
//  Copyright (C) Michigan State University, 2016.
//  Released under the MIT Software license; see doc/LICENSE
//
//
//  This file defines the SignalManager class, which collects sets of Signals to be looked up
//  or manipulated later.

#ifndef EMP_CONTROL_SIGNAL_MANAGER
#define EMP_CONTROL_SIGNAL_MANAGER

#include <string>
#include <unordered_map>

#include "../tools/string_utils.h"

#include "Signal.h"

namespace emp {

  class SignalManager {
  private:
    std::unordered_map<std::string, SignalBase *> signal_map;
    int next_id=1;
    std::string prefix = "emp_signal_";

    // Generate a unique signal name to prevent duplicates.
    inline std::string GenerateSignalName(const std::string & pre="") {
      if (pre=="") return emp::to_string(prefix, next_id++);  // Use default version.
      return emp::to_string(pre, next_id++);
    }
  public:
    SignalManager() = default;
    SignalManager(SignalManager &&) = default;     // Normal juggle is okay for move constructor
    SignalManager(const SignalManager & in) : next_id(in.next_id), prefix(in.prefix) {
      // Copy all signals from input manager.
      for (const auto & x : in.signal_map) {
        signal_map[x.first] = x.second->Clone();
      }
    }
    ~SignalManager() { for (auto & x : signal_map) delete x.second; }

    int GetNextID() const { return next_id; }
    size_t GetSize() const { return signal_map.size(); }

    SignalBase & operator[](const std::string & name) {
      emp_assert(signal_map.find(name) != signal_map.end());
      return *(signal_map[name]);
    }
    const SignalBase & operator[](const std::string & name) const {
      auto it = signal_map.find(name);
      emp_assert(it != signal_map.end());
      return *(it->second);
    }

    template <typename... ARGS>
    auto & Add(const std::string & name) {
      auto * new_signal = new Signal<ARGS...>(name);
      signal_map[name] = new_signal;
      new_signal->signal_id = next_id++;
      return *new_signal;
    }

    void PrintNames(std::ostream & os=std::cout) {
      os << signal_map.size() << " signals found:\n";
      for (auto & x : signal_map) os << "  " << x.first << std::endl;
    }
  };

}

#endif
