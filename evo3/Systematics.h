//  This file is part of Empirical, https://github.com/devosoft/Empirical/
//  Copyright (C) Michigan State University, 2017.
//  Released under the MIT Software license; see doc/LICENSE
//
//
//  Track genotypes, species, clades, or lineages of organisms in a world.
//
//
//  ORG_INFO is usually the genome for an organism, but may have other details like position.
//
//  Options include:
//  * Track lineage yes / no.
//  * Group genotypes yes / no
//  * Group clades (provide a function that determines when a new clade should start)
//  * Prune evolutionary dead-ends (to reduce size of tree).
//  * Backup to file (anything older than a specified level)
//
//
//  Developer notes
//  * Return Ptr<taxon_t> rather than ID to streamline?


#ifndef EMP_EVO_SYSTEMATICS_H
#define EMP_EVO_SYSTEMATICS_H

#include <unordered_set>
#include <set>

#include "../base/Ptr.h"
#include "../tools/set_utils.h"

namespace emp {

  template <typename ORG_INFO>
  class TaxaGroup {
  private:
    using this_t = TaxaGroup<ORG_INFO>;
    using info_t = ORG_INFO;

    const info_t info;        // Details for the organims associated within this taxanomic group.
    const Ptr<this_t> parent; // Pointer to parent group (nullptr if injected)
    size_t num_orgs;          // How many organisms currently exist of this group?
    size_t tot_orgs;          // How many organisms have ever existed of this group?
    size_t num_offspring;     // How many direct offspring groups exist from this one.

  public:
    TaxaGroup(const info_t & _info, Ptr<this_t> _parent=nullptr)
    : info(_info), parent(_parent), num_orgs(0), tot_orgs(0), num_offspring(0) { ; }
    TaxaGroup(const TaxaGroup &) = delete;
    TaxaGroup(TaxaGroup &&) = default;
    TaxaGroup & operator=(const TaxaGroup &) = delete;
    TaxaGroup & operator=(TaxaGroup &&) = default;

    const info_t & GetInfo() const { return info; }
    Ptr<this_t> GetParent() const { return parent; }
    size_t GetNumOrgs() const { return num_orgs; }
    size_t GetTotOrgs() const { return tot_orgs; }
    size_t GetNumOff() const { return num_offspring; }

    void AddOrg() { ++num_orgs; ++tot_orgs; }
    void AddOffspring() { ++num_offspring; }

    // Removals must return true if the taxon needs to continue; false if it should deactivate.
    bool RemoveOrg() {
      emp_assert(num_orgs > 0, num_orgs);
      --num_orgs;

      // If we are out of BOTH organisms and offspring, this TaxaGroup should deactivate.
      if (num_orgs == 0) return num_offspring;
    }
    bool RemoveOffspring() {
      emp_assert(num_offspring > 0);
      --num_offspring;

      // If we are out of BOTH offspring and organisms, this TaxaGroup should deactivate.
      if (num_offspring == 0) return num_orgs;
    }
  };

  template <typename ORG_INFO>
  class Systematics {
  private:
    using taxon_t = TaxaGroup<ORG_INFO>;
    using hash_t = typename Ptr<taxon_t>::hash_t;

    bool store_active;     // Store all of the currently active taxa?
    bool store_ancestors;  // Store all of the direct ancestors from living taxa?
    bool store_outside;    // Store taxa that are extinct with no living descendants?
    bool archive;          // Set to true if we are supposed to do any archiving of extinct taxa.

    std::unordered_set< Ptr<taxon_t>, hash_t > active_taxa;
    std::unordered_set< Ptr<taxon_t>, hash_t > ancestor_taxa;
    std::unordered_set< Ptr<taxon_t>, hash_t > outside_taxa;

    void RemoveOffspring(Ptr<taxon_t> taxon) {
      if (!taxon) return;                          // Not tracking this taxon.
      bool p_active = taxon->RemoveOffspring();    // Cascade up
      if (p_active == false) MarkOutside(taxon);    // If we're out of offspring, now outside.
    }

    // Mark a taxon extinct if there are no more living members.  There may be descendants.
    void MarkExtinct(Ptr<taxon_t> taxon) {
      emp_assert(taxon);
      emp_assert(taxon->GetNumOrgs() == 0);
      if (store_active) active_taxa.remove(taxon);
      if (!archive) {   // If we don't archive taxa, delete them.
        taxon.Delete();
        return;
      }

      // Otherwise, figure out how we're supposed to store them.
      if (taxon->GetNumOff()) {
        // There are offspring taxa, so store as an ancestor (if we're supposed to).
        if (store_ancestors) ancestor_taxa.insert(taxon);
      } else {
        // The are no offspring; store as an outside taxa or delete.
        RemoveOffspring(taxon->GetParent());            // Recurse to parent.
        if (store_outside) outside_taxa.insert(taxon);  // If we're supposed to store, do so.
        else taxon.Delete();                            // Otherwise delete this taxon.
      }
    }

    // A taxon is "outside" the tree when there are not living members AND no living descendants.
    // void MarkOutside(Ptr<taxon_t> taxon) {
    //   emp_assert(taxon);
    //   emp_assert(taxon->GetNumOrgs() == 0);
    //   emp_assert(taxon->GetNumOff() == 0);
    //   if (store_ancestor) ancestor_taxa.remove(taxon); // Remove taxon from ancestor set.
    //   RemoveOffspring(taxon->GetParent());             // Recurse to parent.
    //   if (store_outside) outside_taxa.insert(taxon);   // If we're supposed to store, do so.
    //   else taxon.Delete();                             // Otherwise delete this taxon.
    // }

  public:
    Systematics(bool _active=true, bool _anc=false, bool _all=false)
      : store_active(_active), store_ancestors(_anc), store_outside(_all)
      , archive(store_ancestors || store_outside)
      , active_taxa(), ancestor_taxa(), outside_taxa() { ; }
    Systematics(const Systematics &) = delete;
    Systematics(Systematics &&) = default;
    ~Systematics() {
      for (auto x : active_taxa) x.Delete();
      for (auto x : ancestor_taxa) x.Delete();
      for (auto x : outside_taxa) x.Delete();
      active_taxa.clear();
      ancestor_taxa.clear();
      outside_taxa.clear();
    }

    /// Add information about a newly-injected taxon; return unique taxon pointer.
    Ptr<taxon_t> InjectOrg(const ORG_INFO & info) {
      Ptr<taxon_t> cur_taxon = NewPtr<taxon_t>(info);
      if (store_active) active_taxa.insert(cur_taxon);
      cur_taxon->AddOrg();
      return cur_taxon;
    }

    /// Add information about a new organism; return a pointer for the associated taxon.
    Ptr<taxon_t> AddOrg(Ptr<taxon_t> parent, const ORG_INFO & info) {
      emp_assert(parent);
      emp_assert( Has(active_taxa, parent) );
      if (parent->GetInfo() == info) {   // Adding another org of this taxon.
        parent->AddOrg();
        return parent;
      }
      // Otherwise, this is a new taxon!
      Ptr<taxon_t> cur_taxon = NewPtr<taxon_t>(info, parent);
      if (store_active) active_taxa.insert(cur_taxon);
      cur_taxon->AddOrg();
      return cur_taxon;
    }

    /// Remove an instance of an organism; track when it's gone.
    bool RemoveOrg(Ptr<taxon_t> taxon) {
      emp_assert(taxon);
      emp_assert(Has(active_taxa, taxon));
      const bool active = taxon->RemoveOrg();
      if (active == false) MarkExtinct(taxon);
    }

    /// Climb up a lineage...
    Ptr<taxon_t> Parent(Ptr<taxon_t> taxon) const {
      emp_assert(taxon);
      emp_assert(Has(active_taxa, taxon));
      return taxon->GetParent();
    }
  };

}

#endif