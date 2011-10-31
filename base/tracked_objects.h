// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_TRACKED_OBJECTS_H_
#define BASE_TRACKED_OBJECTS_H_
#pragma once

#include <map>
#include <stack>
#include <string>
#include <vector>

#include "base/base_export.h"
#include "base/location.h"
#include "base/time.h"
#include "base/synchronization/lock.h"
#include "base/threading/thread_local_storage.h"
#include "base/tracking_info.h"
#include "base/values.h"

#if defined(OS_WIN)
#include <mmsystem.h>  // Declare timeGetTime();
#endif

// TrackedObjects provides a database of stats about objects (generally Tasks)
// that are tracked.  Tracking means their birth, death, duration, birth thread,
// death thread, and birth place are recorded.  This data is carefully spread
// across a series of objects so that the counts and times can be rapidly
// updated without (usually) having to lock the data, and hence there is usually
// very little contention caused by the tracking.  The data can be viewed via
// the about:tracking URL, with a variety of sorting and filtering choices.
//
// These classes serve as the basis of a profiler of sorts for the Tasks system.
// As a result, design decisions were made to maximize speed, by minimizing
// recurring allocation/deallocation, lock contention and data copying.  In the
// "stable" state, which is reached relatively quickly, there is no separate
// marginal allocation cost associated with construction or destruction of
// tracked objects, no locks are generally employed, and probably the largest
// computational cost is associated with obtaining start and stop times for
// instances as they are created and destroyed.
//
// The following describes the lifecycle of tracking an instance.
//
// First off, when the instance is created, the FROM_HERE macro is expanded
// to specify the birth place (file, line, function) where the instance was
// created.  That data is used to create a transient Location instance
// encapsulating the above triple of information.  The strings (like __FILE__)
// are passed around by reference, with the assumption that they are static, and
// will never go away.  This ensures that the strings can be dealt with as atoms
// with great efficiency (i.e., copying of strings is never needed, and
// comparisons for equality can be based on pointer comparisons).
//
// Next, a Births instance is created for use ONLY on the thread where this
// instance was created.  That Births instance records (in a base class
// BirthOnThread) references to the static data provided in a Location instance,
// as well as a pointer specifying the thread on which the birth takes place.
// Hence there is at most one Births instance for each Location on each thread.
// The derived Births class contains slots for recording statistics about all
// instances born at the same location.  Statistics currently include only the
// count of instances constructed.
//
// Since the base class BirthOnThread contains only constant data, it can be
// freely accessed by any thread at any time (i.e., only the statistic needs to
// be handled carefully, and stats are updated exclusively on the birth thread).
//
// For Tasks, having now either constructed or found the Births instance
// described above, a pointer to the Births instance is then recorded into the
// PendingTask structure in MessageLoop.  This fact alone is very useful in
// debugging, when there is a question of where an instance came from.  In
// addition, the birth time is also recorded and used to later evaluate the
// lifetime duration of the whole Task.  As a result of the above embedding, we
// can find out a Task's location of birth, and thread of birth, without using
// any locks, as all that data is constant across the life of the process.
//
// The above work *could* also be done for any other object as well by calling
// TallyABirthIfActive() and TallyRunOnNamedThreadIfTracking() as appropriate.
//
// The amount of memory used in the above data structures depends on how many
// threads there are, and how many Locations of construction there are.
// Fortunately, we don't use memory that is the product of those two counts, but
// rather we only need one Births instance for each thread that constructs an
// instance at a Location. In many cases, instances are only created on one
// thread, so the memory utilization is actually fairly restrained.
//
// Lastly, when an instance is deleted, the final tallies of statistics are
// carefully accumulated.  That tallying wrties into slots (members) in a
// collection of DeathData instances.  For each birth place Location that is
// destroyed on a thread, there is a DeathData instance to record the additional
// death count, as well as accumulate the run-time and queue-time durations for
// the instance as it is destroyed (dies).  By maintaining a single place to
// aggregate this running sum *only* for the given thread, we avoid the need to
// lock such DeathData instances. (i.e., these accumulated stats in a DeathData
// instance are exclusively updated by the singular owning thread).
//
// With the above lifecycle description complete, the major remaining detail is
// explaining how each thread maintains a list of DeathData instances, and of
// Births instances, and is able to avoid additional (redundant/unnecessary)
// allocations.
//
// Each thread maintains a list of data items specific to that thread in a
// ThreadData instance (for that specific thread only).  The two critical items
// are lists of DeathData and Births instances.  These lists are maintained in
// STL maps, which are indexed by Location. As noted earlier, we can compare
// locations very efficiently as we consider the underlying data (file,
// function, line) to be atoms, and hence pointer comparison is used rather than
// (slow) string comparisons.
//
// To provide a mechanism for iterating over all "known threads," which means
// threads that have recorded a birth or a death, we create a singly linked list
// of ThreadData instances. Each such instance maintains a pointer to the next
// one.  A static member of ThreadData provides a pointer to the first item on
// this global list, and access via that all_thread_data_list_head_ item
// requires the use of the list_lock_.
// When new ThreadData instances is added to the global list, it is pre-pended,
// which ensures that any prior acquisition of the list is valid (i.e., the
// holder can iterate over it without fear of it changing, or the necessity of
// using an additional lock.  Iterations are actually pretty rare (used
// primarilly for cleanup, or snapshotting data for display), so this lock has
// very little global performance impact.
//
// The above description tries to define the high performance (run time)
// portions of these classes.  After gathering statistics, calls instigated
// by visiting about:tracking will assemble and aggregate data for display. The
// following data structures are used for producing such displays.  They are
// not performance critical, and their only major constraint is that they should
// be able to run concurrently with ongoing augmentation of the birth and death
// data.
//
// For a given birth location, information about births are spread across data
// structures that are asynchronously changing on various threads.  For display
// purposes, we need to construct Snapshot instances for each combination of
// birth thread, death thread, and location, along with the count of such
// lifetimes.  We gather such data into a Snapshot instances, so that such
// instances can be sorted and aggregated (and remain frozen during our
// processing).  Snapshot instances use pointers to constant portions of the
// birth and death datastructures, but have local (frozen) copies of the actual
// statistics (birth count, durations, etc. etc.).
//
// A DataCollector is a container object that holds a set of Snapshots. The
// statistics in a snapshot are gathered asynhcronously relative to their
// ongoing updates.  It is possible, though highly unlikely, that stats such
// as a 64bit counter could incorrectly recorded by this process. The advantage
// to having fast (non-atomic) updates of the data outweighs the minimal risk
// of a singular corrupt statistic snapshot (only the snapshot could be corrupt,
// not the underlying and ongoing stistic).  In constrast, pointer data that is
// accessed during snapshotting is completely invariant, and hence is perfectly
// acquired (i.e., no potential corruption, and no risk of a bad memory
// reference).
//
// After an array of Snapshots instances are colleted into a DataCollector, they
// need to be prepared for display our output. We currently implement a direct
// renderin to HTML, but we will soon also have a JSON serialization as well.

// For direct HTML display, the data must be sorted, and possibly aggregated
// (example: how many threads are in a specific consecutive set of Snapshots?
// What was the total birth count for that set? etc.).  Aggregation instances
// collect running sums of any set of snapshot instances, and are used to print
// sub-totals in an about:tracking page.
//
// TODO(jar): I need to store DataCollections, and provide facilities for taking
// the difference between two gathered DataCollections.  For now, I'm just
// adding a hack that Reset()'s to zero all counts and stats.  This is also
// done in a slighly thread-unsafe fashion, as the reseting is done
// asynchronously relative to ongoing updates, and worse yet, some data fields
// are 64bit quantities, and are not atomicly accessed (reset or incremented
// etc.).  For basic profiling, this will work "most of the time," and should be
// sufficient... but storing away DataCollections is the "right way" to do this.

class MessageLoop;

namespace tracked_objects {

//------------------------------------------------------------------------------

#define USE_FAST_TIME_CLASS_FOR_DURATION_CALCULATIONS

#if defined(USE_FAST_TIME_CLASS_FOR_DURATION_CALCULATIONS)

// TimeTicks maintains a wasteful 64 bits of data (we need less than 32), and on
// windows, a 64 bit timer is expensive to even obtain. We use a simple
// millisecond counter for most of our time values, as well as millisecond units
// of duration between those values.  This means we can only handle durations
// up to 49 days (range), or 24 days (non-negative time durations).
// We only define enough methods to service the needs of the tracking classes,
// and our interfaces are modeled after what TimeTicks and TimeDelta use (so we
// can swap them into place if we want to use the "real" classes).

class BASE_EXPORT Duration {  // Similar to base::TimeDelta.
 public:
  Duration() : ms_(0) {}

  Duration& operator+=(const Duration& other) {
    ms_ += other.ms_;
    return *this;
  }

  Duration operator+(const Duration& other) const {
    return Duration(ms_ + other.ms_);
  }

  bool operator==(const Duration& other) const { return ms_ == other.ms_; }
  bool operator!=(const Duration& other) const { return ms_ != other.ms_; }
  bool operator>(const Duration& other) const { return ms_ > other.ms_; }

  static Duration FromMilliseconds(int ms) { return Duration(ms); }

  int32 InMilliseconds() const { return ms_; }

 private:
  friend class TrackedTime;
  explicit Duration(int32 duration) : ms_(duration) {}

  // Internal time is stored directly in milliseconds.
  int32 ms_;
};

class BASE_EXPORT TrackedTime {  // Similar to base::TimeTicks.
 public:
  TrackedTime() : ms_(0) {}
  explicit TrackedTime(const base::TimeTicks& time)
      : ms_((time - base::TimeTicks()).InMilliseconds()) {
  }

  static TrackedTime Now() {
#if defined(OS_WIN)
    // Use lock-free accessor to 32 bit time.
    // Note that TimeTicks::Now() is built on this, so we have "compatible"
    // times when we down-convert a TimeTicks sample.
    // TODO(jar): Surface this interface via something in base/time.h.
    return TrackedTime(static_cast<int32>(::timeGetTime()));
#else
    // Posix has nice cheap 64 bit times, so we just down-convert it.
    return TrackedTime(base::TimeTicks::Now());
#endif  // OS_WIN
  }

  Duration operator-(const TrackedTime& other) const {
    return Duration(ms_ - other.ms_);
  }

  TrackedTime operator+(const Duration& other) const {
    return TrackedTime(ms_ + other.ms_);
  }

  bool is_null() const { return ms_ == 0; }

 private:
  friend class Duration;
  explicit TrackedTime(int32 ms) : ms_(ms) {}

  // Internal duration is stored directly in milliseconds.
  uint32 ms_;
};

#else

// Just use full 64 bit time calculations, and the slower TimeTicks::Now().
typedef base::TimeTicks TrackedTime;
typedef base::TimeDelta Duration;

#endif  // USE_FAST_TIME_CLASS_FOR_DURATION_CALCULATIONS

//------------------------------------------------------------------------------
// For a specific thread, and a specific birth place, the collection of all
// death info (with tallies for each death thread, to prevent access conflicts).
class ThreadData;
class BASE_EXPORT BirthOnThread {
 public:
  BirthOnThread(const Location& location, const ThreadData& current);

  const Location location() const { return location_; }
  const ThreadData* birth_thread() const { return birth_thread_; }

 private:
  // File/lineno of birth.  This defines the essence of the task, as the context
  // of the birth (construction) often tell what the item is for.  This field
  // is const, and hence safe to access from any thread.
  const Location location_;

  // The thread that records births into this object.  Only this thread is
  // allowed to update birth_count_ (which changes over time).
  const ThreadData* const birth_thread_;

  DISALLOW_COPY_AND_ASSIGN(BirthOnThread);
};

//------------------------------------------------------------------------------
// A class for accumulating counts of births (without bothering with a map<>).

class BASE_EXPORT Births: public BirthOnThread {
 public:
  Births(const Location& location, const ThreadData& current);

  int birth_count() const { return birth_count_; }

  // When we have a birth we update the count for this BirhPLace.
  void RecordBirth() { ++birth_count_; }

  // When a birthplace is changed (updated), we need to decrement the counter
  // for the old instance.
  void ForgetBirth() { --birth_count_; }  // We corrected a birth place.

  // Hack to quickly reset all counts to zero.
  void Clear() { birth_count_ = 0; }

 private:
  // The number of births on this thread for our location_.
  int birth_count_;

  DISALLOW_COPY_AND_ASSIGN(Births);
};

//------------------------------------------------------------------------------
// Basic info summarizing multiple destructions of a tracked object with a
// single birthplace (fixed Location).  Used both on specific threads, and also
// in snapshots when integrating assembled data.

class BASE_EXPORT DeathData {
 public:
  // Default initializer.
  DeathData() : count_(0) {}

  // When deaths have not yet taken place, and we gather data from all the
  // threads, we create DeathData stats that tally the number of births without
  // a corrosponding death.
  explicit DeathData(int count) : count_(count) {}

  // Update stats for a task destruction (death) that had a Run() time of
  // |duration|, and has had a queueing delay of |queue_duration|.
  void RecordDeath(const Duration& queue_duration,
                   const Duration& run_duration);

  // Metrics accessors.
  int count() const { return count_; }
  Duration run_duration() const { return run_duration_; }
  int AverageMsRunDuration() const;
  Duration queue_duration() const { return queue_duration_; }
  int AverageMsQueueDuration() const;

  // Accumulate metrics from other into this.  This method is never used on
  // realtime statistics, and only used in snapshots and aggregatinos.
  void AddDeathData(const DeathData& other);

  // Simple print of internal state for use in line of HTML.
  void WriteHTML(std::string* output) const;

  // Construct a DictionaryValue instance containing all our stats. The caller
  // assumes ownership of the returned instance.
  base::DictionaryValue* ToValue() const;

  // Reset all tallies to zero. This is used as a hack on realtime data.
  void Clear();

 private:
  int count_;                       // Number of destructions.
  Duration run_duration_;    // Sum of all Run()time durations.
  Duration queue_duration_;  // Sum of all queue time durations.
};

//------------------------------------------------------------------------------
// A temporary collection of data that can be sorted and summarized.  It is
// gathered (carefully) from many threads.  Instances are held in arrays and
// processed, filtered, and rendered.
// The source of this data was collected on many threads, and is asynchronously
// changing.  The data in this instance is not asynchronously changing.

class BASE_EXPORT Snapshot {
 public:
  // When snapshotting a full life cycle set (birth-to-death), use this:
  Snapshot(const BirthOnThread& birth_on_thread, const ThreadData& death_thread,
           const DeathData& death_data);

  // When snapshotting a birth, with no death yet, use this:
  Snapshot(const BirthOnThread& birth_on_thread, int count);

  const ThreadData* birth_thread() const { return birth_->birth_thread(); }
  const Location location() const { return birth_->location(); }
  const BirthOnThread& birth() const { return *birth_; }
  const ThreadData* death_thread() const {return death_thread_; }
  const DeathData& death_data() const { return death_data_; }
  const std::string DeathThreadName() const;

  int count() const { return death_data_.count(); }
  Duration run_duration() const { return death_data_.run_duration(); }
  Duration queue_duration() const { return death_data_.queue_duration(); }
  int AverageMsRunDuration() const {
    return death_data_.AverageMsRunDuration();
  }
  int AverageMsQueueDuration() const {
    return death_data_.AverageMsQueueDuration();
  }

  // Emit contents for use in a line of HTML
  void WriteHTML(std::string* output) const;

  // Construct a DictionaryValue instance containing all our data recursively.
  // The caller assumes ownership of the memory in the returned instance.
  base::DictionaryValue* ToValue() const;

 private:
  const BirthOnThread* birth_;  // Includes Location and birth_thread.
  const ThreadData* death_thread_;
  DeathData death_data_;
};

//------------------------------------------------------------------------------
// DataCollector is a container class for Snapshot and BirthOnThread count
// items.

class BASE_EXPORT DataCollector {
 public:
  typedef std::vector<Snapshot> Collection;

  // Construct with a list of how many threads should contribute.  This helps us
  // determine (in the async case) when we are done with all contributions.
  DataCollector();
  ~DataCollector();

  // Adds all stats from the indicated thread into our arrays.  This function
  // uses locks at the lowest level (when accessing the underlying maps which
  // could change when not locked), and can be called from any threads.
  void Append(const ThreadData& thread_data);

  // After the accumulation phase, the following accessor is used to process the
  // data (i.e., sort it, filter it, etc.).
  Collection* collection();

  // Adds entries for all the remaining living objects (objects that have
  // tallied a birth, but have not yet tallied a matching death, and hence must
  // be either running, queued up, or being held in limbo for future posting).
  // This should be called after all known ThreadData instances have been
  // processed using Append().
  void AddListOfLivingObjects();

  // Generates a ListValue representation of the vector of snapshots. The caller
  // assumes ownership of the memory in the returned instance.
  base::ListValue* ToValue() const;

 private:
  typedef std::map<const BirthOnThread*, int> BirthCount;

  // The array that we collect data into.
  Collection collection_;

  // The total number of births recorded at each location for which we have not
  // seen a death count.  This map changes as we do Append() calls, and is later
  // used by AddListOfLivingObjects() to gather up unaccounted for births.
  BirthCount global_birth_count_;

  DISALLOW_COPY_AND_ASSIGN(DataCollector);
};

//------------------------------------------------------------------------------
// Aggregation contains summaries (totals and subtotals) of groups of Snapshot
// instances to provide printing of these collections on a single line.
// We generally provide an aggregate total for the entire list, as well as
// aggregate subtotals for groups of stats (example: group of all lives that
// died on the specific thread).

class BASE_EXPORT Aggregation: public DeathData {
 public:
  Aggregation();
  ~Aggregation();

  void AddDeathSnapshot(const Snapshot& snapshot);
  void AddBirths(const Births& births);
  void AddBirth(const BirthOnThread& birth);
  void AddBirthPlace(const Location& location);
  void WriteHTML(std::string* output) const;
  void Clear();

 private:
  int birth_count_;
  std::map<std::string, int> birth_files_;
  std::map<Location, int> locations_;
  std::map<const ThreadData*, int> birth_threads_;
  DeathData death_data_;
  std::map<const ThreadData*, int> death_threads_;

  DISALLOW_COPY_AND_ASSIGN(Aggregation);
};

//------------------------------------------------------------------------------
// Comparator is a class that supports the comparison of Snapshot instances.
// An instance is actually a list of chained Comparitors, that can provide for
// arbitrary ordering.  The path portion of an about:tracking URL is translated
// into such a chain, which is then used to order Snapshot instances in a
// vector.  It orders them into groups (for aggregation), and can also order
// instances within the groups (for detailed rendering of the instances in an
// aggregation).

class BASE_EXPORT Comparator {
 public:
  // Selector enum is the token identifier for each parsed keyword, most of
  // which specify a sort order.
  // Since it is not meaningful to sort more than once on a specific key, we
  // use bitfields to accumulate what we have sorted on so far.
  enum Selector {
    // Sort orders.
    NIL = 0,
    BIRTH_THREAD = 1,
    DEATH_THREAD = 2,
    BIRTH_FILE = 4,
    BIRTH_FUNCTION = 8,
    BIRTH_LINE = 16,
    COUNT = 32,
    AVERAGE_RUN_DURATION = 64,
    TOTAL_RUN_DURATION = 128,
    AVERAGE_QUEUE_DURATION = 256,
    TOTAL_QUEUE_DURATION = 512,

    // Imediate action keywords.
    RESET_ALL_DATA = -1,
    UNKNOWN_KEYWORD = -2,
  };

  explicit Comparator();

  // Reset the comparator to a NIL selector.  Clear() and recursively delete any
  // tiebreaker_ entries.  NOTE: We can't use a standard destructor, because
  // the sort algorithm makes copies of this object, and then deletes them,
  // which would cause problems (either we'd make expensive deep copies, or we'd
  // do more thna one delete on a tiebreaker_.
  void Clear();

  // The less() operator for sorting the array via std::sort().
  bool operator()(const Snapshot& left, const Snapshot& right) const;

  void Sort(DataCollector::Collection* collection) const;

  // Check to see if the items are sort equivalents (should be aggregated).
  bool Equivalent(const Snapshot& left, const Snapshot& right) const;

  // Check to see if all required fields are present in the given sample.
  bool Acceptable(const Snapshot& sample) const;

  // A comparator can be refined by specifying what to do if the selected basis
  // for comparison is insufficient to establish an ordering.  This call adds
  // the indicated attribute as the new "least significant" basis of comparison.
  void SetTiebreaker(Selector selector, const std::string& required);

  // Indicate if this instance is set up to sort by the given Selector, thereby
  // putting that information in the SortGrouping, so it is not needed in each
  // printed line.
  bool IsGroupedBy(Selector selector) const;

  // Using the tiebreakers as set above, we mostly get an ordering, with some
  // equivalent groups.  If those groups are displayed (rather than just being
  // aggregated, then the following is used to order them (within the group).
  void SetSubgroupTiebreaker(Selector selector);

  // Translate a keyword and restriction in URL path to a selector for sorting.
  void ParseKeyphrase(const std::string& key_phrase);

  // Parse a query to decide on sort ordering.
  bool ParseQuery(const std::string& query);

  // Output a header line that can be used to indicated what items will be
  // collected in the group.  It lists all (potentially) tested attributes and
  // their values (in the sample item).
  bool WriteSortGrouping(const Snapshot& sample, std::string* output) const;

  // Output a sample, with SortGroup details not displayed.
  void WriteSnapshotHTML(const Snapshot& sample, std::string* output) const;

 private:
  // The selector directs this instance to compare based on the specified
  // members of the tested elements.
  enum Selector selector_;

  // Translate a path keyword into a selector.  This is a slow implementation,
  // but this is rarely done, and only for HTML presentations.
  static Selector FindSelector(const std::string& keyword);

  // For filtering into acceptable and unacceptable snapshot instance, the
  // following is required to be a substring of the selector_ field.
  std::string required_;

  // If this instance can't decide on an ordering, we can consult a tie-breaker
  // which may have a different basis of comparison.
  Comparator* tiebreaker_;

  // We or together all the selectors we sort on (not counting sub-group
  // selectors), so that we can tell if we've decided to group on any given
  // criteria.
  int combined_selectors_;

  // Some tiebreakrs are for subgroup ordering, and not for basic ordering (in
  // preparation for aggregation).  The subgroup tiebreakers are not consulted
  // when deciding if two items are in equivalent groups.  This flag tells us
  // to ignore the tiebreaker when doing Equivalent() testing.
  bool use_tiebreaker_for_sort_only_;
};

//------------------------------------------------------------------------------
// For each thread, we have a ThreadData that stores all tracking info generated
// on this thread.  This prevents the need for locking as data accumulates.
// We use ThreadLocalStorage to quickly identfy the current ThreadData context.
// We also have a linked list of ThreadData instances, and that list is used to
// harvest data from all existing instances.

class BASE_EXPORT ThreadData {
 public:
  // Current allowable states of the tracking system.  The states can vary
  // between ACTIVE and DEACTIVATED, but can never go back to UNINITIALIZED.
  enum Status {
    UNINITIALIZED,
    ACTIVE,
    DEACTIVATED,
  };

  typedef std::map<Location, Births*> BirthMap;
  typedef std::map<const Births*, DeathData> DeathMap;

  // Initialize the current thread context with a new instance of ThreadData.
  // This is used by all threads that have names, and should be explicitly
  // set *before* any births on the threads have taken place.  It is generally
  // only used by the message loop, which has a well defined thread name.
  static void InitializeThreadContext(const std::string& suggested_name);

  // Using Thread Local Store, find the current instance for collecting data.
  // If an instance does not exist, construct one (and remember it for use on
  // this thread.
  // This may return NULL if the system is disabled for any reason.
  static ThreadData* Get();

  // For a given (unescaped) about:tracking query, develop resulting HTML, and
  // append to output.
  static void WriteHTML(const std::string& query, std::string* output);

  // For a given accumulated array of results, use the comparator to sort and
  // subtotal, writing the results to the output.
  static void WriteHTMLTotalAndSubtotals(
      const DataCollector::Collection& match_array,
      const Comparator& comparator, std::string* output);

  // Constructs a DictionaryValue instance containing all recursive results in
  // our process.  The caller assumes ownership of the memory in the returned
  // instance.
  static base::DictionaryValue* ToValue();

  // Finds (or creates) a place to count births from the given location in this
  // thread, and increment that tally.
  // TallyABirthIfActive will returns NULL if the birth cannot be tallied.
  static Births* TallyABirthIfActive(const Location& location);

  // Records the end of a timed run of an object.  The |completed_task| contains
  // a pointer to a Births, the time_posted, and a delayed_start_time if any.
  // The |start_of_run| indicates when we started to perform the run of the
  // task.  The delayed_start_time is non-null for tasks that were posted as
  // delayed tasks, and it indicates when the task should have run (i.e., when
  // it should have posted out of the timer queue, and into the work queue.
  // The |end_of_run| was just obtained by a call to Now() (just after the task
  // finished). It is provided as an argument to help with testing.
  static void TallyRunOnNamedThreadIfTracking(
      const base::TrackingInfo& completed_task,
      const TrackedTime& start_of_run,
      const TrackedTime& end_of_run);

  // Record the end of a timed run of an object.  The |birth| is the record for
  // the instance, the |time_posted| records that instant, which is presumed to
  // be when the task was posted into a queue to run on a worker thread.
  // The |start_of_run| is when the worker thread started to perform the run of
  // the task.
  // The |end_of_run| was just obtained by a call to Now() (just after the task
  // finished).
  static void TallyRunOnWorkerThreadIfTracking(
      const Births* birth,
      const TrackedTime& time_posted,
      const TrackedTime& start_of_run,
      const TrackedTime& end_of_run);

  const std::string thread_name() const { return thread_name_; }

  // ---------------------
  // The following functions should all be private, and are only public because
  // the collection is done externally.  We need to relocate that code from the
  // collection class into this class, and then all these methods can be made
  // private.
  // (Thread safe) Get start of list of all ThreadData instances.
  static ThreadData* first();
  // Iterate through the null terminated list of ThreadData instances.
  ThreadData* next() const { return next_; }
  // Using our lock, make a copy of the specified maps.  These calls may arrive
  // from non-local threads, and are used to quickly scan data from all threads
  // in order to build an HTML page for about:tracking.
  void SnapshotBirthMap(BirthMap *output) const;
  void SnapshotDeathMap(DeathMap *output) const;
  // -------- end of should be private methods.

  // Hack: asynchronously clear all birth counts and death tallies data values
  // in all ThreadData instances.  The numerical (zeroing) part is done without
  // use of a locks or atomics exchanges, and may (for int64 values) produce
  // bogus counts VERY rarely.
  static void ResetAllThreadData();

  // Initializes all statics if needed (this initialization call should be made
  // while we are single threaded). Returns false if unable to initialize.
  static bool Initialize();

  // Sets internal status_ to either become ACTIVE, or DEACTIVATED,
  // based on argument being true or false respectively.
  // If tracking is not compiled in, this function will return false.
  static bool InitializeAndSetTrackingStatus(bool status);
  static bool tracking_status();

  // Provide a time function that does nothing (runs fast) when we don't have
  // the profiler enabled.  It will generally be optimized away when it is
  // ifdef'ed to be small enough (allowing the profiler to be "compiled out" of
  // the code).
  static TrackedTime Now();

  // Cleans up data structures, and returns statics to near pristine (mostly
  // uninitialized) state.  If there is any chance that other threads are still
  // using the data structures, then the |leak| argument should be passed in as
  // true, and the data structures (birth maps, death maps, ThreadData
  // insntances, etc.) will be leaked and not deleted.  If you have joined all
  // threads since the time that InitializeAndSetTrackingStatus() was called,
  // then you can pass in a |leak| value of false, and this function will
  // delete recursively all data structures, starting with the list of
  // ThreadData instances.
  static void ShutdownSingleThreadedCleanup(bool leak);

 private:
  typedef std::stack<const ThreadData*> ThreadDataPool;

  // Worker thread construction creates a name since there is none.
  ThreadData();
  // Message loop based construction should provide a name.
  explicit ThreadData(const std::string& suggested_name);

  ~ThreadData();

  // Push this instance to the head of all_thread_data_list_head_, linking it to
  // the previous head.  This is performed after each construction, and leaves
  // the instance permanently on that list.
  void PushToHeadOfList();

  // In this thread's data, record a new birth.
  Births* TallyABirth(const Location& location);

  // Find a place to record a death on this thread.
  void TallyADeath(const Births& birth,
                   const Duration& queue_duration,
                   const Duration& duration);

  // Using our lock to protect the iteration, Clear all birth and death data.
  void Reset();

  // This method is called by the TLS system when a thread terminates.
  // The argument may be NULL if this thread has never tracked a birth or death.
  static void OnThreadTermination(void* thread_data);

  // This method should be called when a worker thread terminates, so that we
  // can save all the thread data into a cache of reusable ThreadData instances.
  void OnThreadTerminationCleanup() const;

  // We use thread local store to identify which ThreadData to interact with.
  static base::ThreadLocalStorage::Slot tls_index_;

  // Link to the most recently created instance (starts a null terminated list).
  // The list is traversed by about:tracking when it needs to snapshot data.
  // This is only accessed while list_lock_ is held.
  static ThreadData* all_thread_data_list_head_;
  // Set of ThreadData instances for use with worker threads. When a worker
  // thread is done (terminating), we push it into this pool.  When a new worker
  // thread is created, we first try to re-use a ThreadData instance from the
  // pool, and if none are available, construct a new one.
  // This is only accessed while list_lock_ is held.
  static ThreadDataPool* unregistered_thread_data_pool_;
  // The next available thread number.  This should only be accessed when the
  // list_lock_ is held.
  static int thread_number_counter_;
  // Incarnation sequence number, indicating how many times (during unittests)
  // we've either transitioned out of UNINITIALIZED, or into that state.  This
  // value is only accessed while the list_lock_ is held.
  static int incarnation_counter_;
  // Protection for access to all_thread_data_list_head_, and to
  // unregistered_thread_data_pool_.  This lock is leaked at shutdown.
  static base::Lock* list_lock_;

  // Record of what the incarnation_counter_ was when this instance was created.
  // If the incarnation_counter_ has changed, then we avoid pushing into the
  // pool (this is only critical in tests which go through multiple
  // incarations).
  int incarnation_count_for_pool_;

  // We set status_ to SHUTDOWN when we shut down the tracking service.
  static Status status_;

  // Link to next instance (null terminated list). Used to globally track all
  // registered instances (corresponds to all registered threads where we keep
  // data).
  ThreadData* next_;

  // The name of the thread that is being recorded.  If this thread has no
  // message_loop, then this is a worker thread, with a sequence number postfix.
  std::string thread_name_;

  // Indicate if this is a worker thread, and the ThreadData contexts should be
  // stored in the unregistered_thread_data_pool_ when not in use.
  bool is_a_worker_thread_;

  // A map used on each thread to keep track of Births on this thread.
  // This map should only be accessed on the thread it was constructed on.
  // When a snapshot is needed, this structure can be locked in place for the
  // duration of the snapshotting activity.
  BirthMap birth_map_;

  // Similar to birth_map_, this records informations about death of tracked
  // instances (i.e., when a tracked instance was destroyed on this thread).
  // It is locked before changing, and hence other threads may access it by
  // locking before reading it.
  DeathMap death_map_;

  // Lock to protect *some* access to BirthMap and DeathMap.  The maps are
  // regularly read and written on this thread, but may only be read from other
  // threads.  To support this, we acquire this lock if we are writing from this
  // thread, or reading from another thread.  For reading from this thread we
  // don't need a lock, as there is no potential for a conflict since the
  // writing is only done from this thread.
  mutable base::Lock lock_;

  DISALLOW_COPY_AND_ASSIGN(ThreadData);
};

//------------------------------------------------------------------------------
// Provide simple way to to start global tracking, and to tear down tracking
// when done.  The design has evolved to *not* do any teardown (and just leak
// all allocated data structures).  As a result, we don't have any code in this
// destructor, and perhaps this whole class should go away.

class BASE_EXPORT AutoTracking {
 public:
  AutoTracking() {
    ThreadData::Initialize();
  }

  ~AutoTracking() {
    // TODO(jar): Consider emitting a CSV dump of the data at this point.  This
    // should be called after the message loops have all terminated (or at least
    // the main message loop is gone), so there is little chance for additional
    // tasks to be Run.
  }

 private:

  DISALLOW_COPY_AND_ASSIGN(AutoTracking);
};

}  // namespace tracked_objects

#endif  // BASE_TRACKED_OBJECTS_H_
