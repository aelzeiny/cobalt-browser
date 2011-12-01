// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Use trace_analyzer::Query and trace_analyzer::TraceAnalyzer to search for
// specific trace events that were generated by the trace_event.h API.
//
// Basic procedure:
// - Get trace events JSON string from base::debug::TraceLog.
// - Create TraceAnalyzer with JSON string.
// - Call TraceAnalyzer::AssociateBeginEndEvents (optional).
// - Call TraceAnalyzer::AssociateEvents (zero or more times).
// - Call TraceAnalyzer::FindEvents with queries to find specific events.
//
// A Query is a boolean expression tree that evaluates to true or false for a
// given trace event. Queries can be combined into a tree using boolean,
// arithmetic and comparison operators that refer to data of an individual trace
// event.
//
// The events are returned as trace_analyzer::TraceEvent objects.
// TraceEvent contains a single trace event's data, as well as a pointer to
// a related trace event. The related trace event is typically the matching end
// of a begin event or the matching begin of an end event.
//
// The following examples use this basic setup code to construct TraceAnalyzer
// with the json trace string retrieved from TraceLog and construct an event
// vector for retrieving events:
//
// TraceAnalyzer analyzer(json_events);
// TraceEventVector events;
//
// During construction, TraceAnalyzer::SetDefaultAssociations is called to
// associate all matching begin/end pairs similar to how they are shown in
// about:tracing.
//
// EXAMPLE 1: Find events named "my_event".
//
// analyzer.FindEvents(Query(EVENT_NAME) == "my_event", &events);
//
// EXAMPLE 2: Find begin events named "my_event" with duration > 1 second.
//
// Query q = (Query(EVENT_NAME) == Query::String("my_event") &&
//            Query(EVENT_PHASE) == Query::Phase(TRACE_EVENT_PHASE_BEGIN) &&
//            Query(EVENT_DURATION) > Query::Double(1000000.0));
// analyzer.FindEvents(q, &events);
//
// EXAMPLE 3: Associating event pairs across threads.
//
// If the test needs to analyze something that starts and ends on different
// threads, the test needs to use INSTANT events. The typical procedure is to
// specify the same unique ID as a TRACE_EVENT argument on both the start and
// finish INSTANT events. Then use the following procedure to associate those
// events.
//
// Step 1: instrument code with custom begin/end trace events.
//   [Thread 1 tracing code]
//   TRACE_EVENT_INSTANT1("test_latency", "timing1_begin", "id", 3);
//   [Thread 2 tracing code]
//   TRACE_EVENT_INSTANT1("test_latency", "timing1_end", "id", 3);
//
// Step 2: associate these custom begin/end pairs.
//   Query begin(Query(EVENT_NAME) == Query::String("timing1_begin"));
//   Query end(Query(EVENT_NAME) == Query::String("timing1_end"));
//   Query match(Query(EVENT_ARG, "id") == Query(OTHER_ARG, "id"));
//   analyzer.AssociateEvents(begin, end, match);
//
// Step 3: search for "timing1_begin" events with existing other event.
//   Query q = (Query(EVENT_NAME) == Query::String("timing1_begin") &&
//              Query(EVENT_HAS_OTHER));
//   analyzer.FindEvents(q, &events);
//
// Step 4: analyze events, such as checking durations.
//   for (size_t i = 0; i < events.size(); ++i) {
//     double duration;
//     EXPECT_TRUE(events[i].GetAbsTimeToOtherEvent(&duration));
//     EXPECT_LT(duration, 1000000.0/60.0); // expect less than 1/60 second.
//   }


#ifndef BASE_TEST_TRACE_EVENT_ANALYZER_H_
#define BASE_TEST_TRACE_EVENT_ANALYZER_H_
#pragma once

#include <map>

#include "base/debug/trace_event.h"
#include "base/memory/ref_counted_memory.h"
#include "base/memory/scoped_ptr.h"

namespace base {
class Value;
}

namespace trace_analyzer {
class QueryNode;

// trace_analyzer::TraceEvent is a more convenient form of the
// base::debug::TraceEvent class to make tracing-based tests easier to write.
struct TraceEvent {
  // ProcessThreadID contains a Process ID and Thread ID.
  struct ProcessThreadID {
    ProcessThreadID() : process_id(0), thread_id(0) {}
    ProcessThreadID(int process_id, int thread_id)
        : process_id(process_id), thread_id(thread_id) {}
    bool operator< (const ProcessThreadID& rhs) const {
      if (process_id != rhs.process_id)
        return process_id < rhs.process_id;
      return thread_id < rhs.thread_id;
    }
    int process_id;
    int thread_id;
  };

  TraceEvent();
  ~TraceEvent();

  bool SetFromJSON(const base::Value* event_value) WARN_UNUSED_RESULT;

  bool operator< (const TraceEvent& rhs) const {
    return timestamp < rhs.timestamp;
  }

  bool has_other_event() const { return other_event; }

  // Returns absolute duration in microseconds between this event and other
  // event. Must have already verified that other_event exists by
  // Query(EVENT_HAS_OTHER) or by calling has_other_event().
  double GetAbsTimeToOtherEvent() const;

  // Return the argument value if it exists and it is a string.
  bool GetArgAsString(const std::string& name, std::string* arg) const;
  // Return the argument value if it exists and it is a number.
  bool GetArgAsNumber(const std::string& name, double* arg) const;

  // Check if argument exists and is string.
  bool HasStringArg(const std::string& name) const;
  // Check if argument exists and is number (double, int or bool).
  bool HasNumberArg(const std::string& name) const;

  // Get known existing arguments as specific types.
  // Useful when you have already queried the argument with
  // Query(HAS_NUMBER_ARG) or Query(HAS_STRING_ARG).
  std::string GetKnownArgAsString(const std::string& name) const;
  double GetKnownArgAsDouble(const std::string& name) const;
  int GetKnownArgAsInt(const std::string& name) const;
  bool GetKnownArgAsBool(const std::string& name) const;

  // Process ID and Thread ID.
  ProcessThreadID thread;

  // Time since epoch in microseconds.
  // Stored as double to match its JSON representation.
  double timestamp;

  base::debug::TraceEventPhase phase;

  std::string category;

  std::string name;

  // All numbers and bool values from TraceEvent args are cast to double.
  // bool becomes 1.0 (true) or 0.0 (false).
  std::map<std::string, double> arg_numbers;

  std::map<std::string, std::string> arg_strings;

  // The other event associated with this event (or NULL).
  const TraceEvent* other_event;
};

// Pass these values to Query to compare with the corresponding member of a
// TraceEvent. Unless otherwise specfied, the usage is Query(ENUM_MEMBER).
enum TraceEventMember {
  EVENT_INVALID,
  // Use these to access the event members:
  EVENT_PID,
  EVENT_TID,
  // Return the timestamp of the event in microseconds since epoch.
  EVENT_TIME,
  // Return the absolute time between event and other event in microseconds.
  // Only works for events with associated BEGIN/END: Query(EVENT_HAS_OTHER).
  EVENT_DURATION,
  EVENT_PHASE,
  EVENT_CATEGORY,
  EVENT_NAME,

  // Evaluates to true if arg exists and is a string.
  // Usage: Query(EVENT_HAS_STRING_ARG, "arg_name")
  EVENT_HAS_STRING_ARG,
  // Evaluates to true if arg exists and is a number.
  // Number arguments include types double, int and bool.
  // Usage: Query(EVENT_HAS_NUMBER_ARG, "arg_name")
  EVENT_HAS_NUMBER_ARG,
  // Evaluates to arg value (string or number).
  // Usage: Query(EVENT_ARG, "arg_name")
  EVENT_ARG,
  // Return true if associated event exists.
  // (Typically BEGIN for END or END for BEGIN).
  EVENT_HAS_OTHER,

  // Access the associated other_event's members:
  OTHER_PID,
  OTHER_TID,
  OTHER_TIME,
  OTHER_PHASE,
  OTHER_CATEGORY,
  OTHER_NAME,

  // Evaluates to true if arg exists and is a string.
  // Usage: Query(EVENT_HAS_STRING_ARG, "arg_name")
  OTHER_HAS_STRING_ARG,
  // Evaluates to true if arg exists and is a number.
  // Number arguments include types double, int and bool.
  // Usage: Query(EVENT_HAS_NUMBER_ARG, "arg_name")
  OTHER_HAS_NUMBER_ARG,
  // Evaluates to arg value (string or number).
  // Usage: Query(EVENT_ARG, "arg_name")
  OTHER_ARG,
};

class Query {
 public:
  // Compare with the given member.
  Query(TraceEventMember member);

  // Compare with the given member argument value.
  Query(TraceEventMember member, const std::string& arg_name);

  Query(const Query& query);

  ~Query();

  // Compare with the given string.
  static Query String(const std::string& str);

  // Compare with the given number.
  static Query Double(double num);
  static Query Int(int32 num);
  static Query Uint(uint32 num);

  // Compare with the given bool.
  static Query Bool(bool boolean);

  // Compare with the given phase.
  static Query Phase(base::debug::TraceEventPhase phase);

  // Compare with the given string pattern. Only works with == and != operators.
  // Example: Query(EVENT_NAME) == Query::Pattern("MyEvent*")
  static Query Pattern(const std::string& pattern);

  // Common queries:

  // Find BEGIN events that have a corresponding END event.
  static Query MatchBeginWithEnd() {
    return (Query(EVENT_PHASE) ==
            Query::Phase(base::debug::TRACE_EVENT_PHASE_BEGIN)) &&
           Query(EVENT_HAS_OTHER);
  }

  // Find BEGIN events of given |name| which also have associated END events.
  static Query MatchBeginName(const std::string& name) {
    return (Query(EVENT_NAME) == name) && MatchBeginWithEnd();
  }

  // Match given Process ID and Thread ID.
  static Query MatchThread(const TraceEvent::ProcessThreadID& thread) {
    return (Query(EVENT_PID) == Query::Int(thread.process_id)) &&
           (Query(EVENT_TID) == Query::Int(thread.thread_id));
  }

  // Match event pair that spans multiple threads.
  static Query MatchCrossThread() {
    return (Query(EVENT_PID) != Query(OTHER_PID)) ||
           (Query(EVENT_TID) != Query(OTHER_TID));
  }

  // Boolean operators:
  Query operator==(const Query& rhs) const;
  Query operator!=(const Query& rhs) const;
  Query operator< (const Query& rhs) const;
  Query operator<=(const Query& rhs) const;
  Query operator> (const Query& rhs) const;
  Query operator>=(const Query& rhs) const;
  Query operator&&(const Query& rhs) const;
  Query operator||(const Query& rhs) const;
  Query operator!() const;

  // Arithmetic operators:
  // Following operators are applied to double arguments:
  Query operator+(const Query& rhs) const;
  Query operator-(const Query& rhs) const;
  Query operator*(const Query& rhs) const;
  Query operator/(const Query& rhs) const;
  Query operator-() const;
  // Mod operates on int64 args (doubles are casted to int64 beforehand):
  Query operator%(const Query& rhs) const;

  // Return true if the given event matches this query tree.
  // This is a recursive method that walks the query tree.
  bool Evaluate(const TraceEvent& event) const;

 private:
  enum Operator {
    OP_INVALID,
    // Boolean operators:
    OP_EQ,
    OP_NE,
    OP_LT,
    OP_LE,
    OP_GT,
    OP_GE,
    OP_AND,
    OP_OR,
    OP_NOT,
    // Arithmetic operators:
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_MOD,
    OP_NEGATE
  };

  enum QueryType {
    QUERY_BOOLEAN_OPERATOR,
    QUERY_ARITHMETIC_OPERATOR,
    QUERY_EVENT_MEMBER,
    QUERY_NUMBER,
    QUERY_STRING
  };

  // Compare with the given string.
  Query(const std::string& str);

  // Compare with the given number.
  Query(double num);

  // Construct a boolean Query that returns (left <binary_op> right).
  Query(const Query& left, const Query& right, Operator binary_op);

  // Construct a boolean Query that returns (<binary_op> left).
  Query(const Query& left, Operator unary_op);

  // Try to compare left_ against right_ based on operator_.
  // If either left or right does not convert to double, false is returned.
  // Otherwise, true is returned and |result| is set to the comparison result.
  bool CompareAsDouble(const TraceEvent& event, bool* result) const;

  // Try to compare left_ against right_ based on operator_.
  // If either left or right does not convert to string, false is returned.
  // Otherwise, true is returned and |result| is set to the comparison result.
  bool CompareAsString(const TraceEvent& event, bool* result) const;

  // Attempt to convert this Query to a double. On success, true is returned
  // and the double value is stored in |num|.
  bool GetAsDouble(const TraceEvent& event, double* num) const;

  // Attempt to convert this Query to a string. On success, true is returned
  // and the string value is stored in |str|.
  bool GetAsString(const TraceEvent& event, std::string* str) const;

  // Evaluate this Query as an arithmetic operator on left_ and right_.
  bool EvaluateArithmeticOperator(const TraceEvent& event,
                                  double* num) const;

  // For QUERY_EVENT_MEMBER Query: attempt to get the value of the Query.
  // The TraceValue will either be TRACE_TYPE_DOUBLE, TRACE_TYPE_STRING,
  // or if requested member does not exist, it will be TRACE_TYPE_UNDEFINED.
  base::debug::TraceValue GetMemberValue(const TraceEvent& event) const;

  // Does this Query represent a value?
  bool is_value() const { return type_ != QUERY_BOOLEAN_OPERATOR; }

  bool is_unary_operator() const {
    return operator_ == OP_NOT || operator_ == OP_NEGATE;
  }

  bool is_comparison_operator() const {
    return operator_ != OP_INVALID && operator_ < OP_AND;
  }

  const Query& left() const;
  const Query& right() const;

  QueryType type_;
  Operator operator_;
  scoped_refptr<QueryNode> left_;
  scoped_refptr<QueryNode> right_;
  TraceEventMember member_;
  double number_;
  std::string string_;
  bool is_pattern_;
};

// Implementation detail:
// QueryNode allows Query to store a ref-counted query tree.
class QueryNode : public base::RefCounted<QueryNode> {
 public:
  explicit QueryNode(const Query& query);
  const Query& query() const { return query_; }

 private:
  friend class base::RefCounted<QueryNode>;
  ~QueryNode();

  Query query_;
};

// TraceAnalyzer helps tests search for trace events.
class TraceAnalyzer {
 public:
  typedef std::vector<const TraceEvent*> TraceEventVector;

  struct Stats {
    double min_us;
    double max_us;
    double mean_us;
    double standard_deviation_us;
  };

  ~TraceAnalyzer();

  // Use trace events from JSON string generated by tracing API.
  // Returns non-NULL if the JSON is successfully parsed.
  static TraceAnalyzer* Create(const std::string& json_events)
                               WARN_UNUSED_RESULT;

  // Associate BEGIN and END events with each other. This allows Query(OTHER_*)
  // to access the associated event and enables Query(EVENT_DURATION).
  // An end event will match the most recent begin event with the same name,
  // category, process ID and thread ID. This matches what is shown in
  // about:tracing.
  void AssociateBeginEndEvents();

  // AssociateEvents can be used to customize event associations by setting the
  // other_event member of TraceEvent. This should be used to associate two
  // INSTANT events.
  //
  // The assumptions are:
  // - |first| events occur before |second| events.
  // - the closest matching |second| event is the correct match.
  //
  // |first|  - Eligible |first| events match this query.
  // |second| - Eligible |second| events match this query.
  // |match|  - This query is run on the |first| event. The OTHER_* EventMember
  //            queries will point to an eligible |second| event. The query
  //            should evaluate to true if the |first|/|second| pair is a match.
  //
  // When a match is found, the pair will be associated by having their
  // other_event member point to each other. AssociateEvents does not clear
  // previous associations, so it is possible to associate multiple pairs of
  // events by calling AssociateEvents more than once with different queries.
  //
  // NOTE: AssociateEvents will overwrite existing other_event associations if
  // the queries pass for events that already had a previous association.
  //
  // After calling FindEvents or FindOneEvent, it is not allowed to call
  // AssociateEvents again.
  void AssociateEvents(const Query& first,
                       const Query& second,
                       const Query& match);

  // Find all events that match query and replace output vector.
  size_t FindEvents(const Query& query, TraceEventVector* output);

  // Helper method: find first event that matches query
  const TraceEvent* FindOneEvent(const Query& query);

  const std::string& GetThreadName(const TraceEvent::ProcessThreadID& thread);

  // Calculate min/max/mean and standard deviation from the times between
  // adjacent events.
  static bool GetRateStats(const TraceEventVector& events, Stats* stats);

 private:
  TraceAnalyzer();

  bool SetEvents(const std::string& json_events) WARN_UNUSED_RESULT;

  // Read metadata (thread names, etc) from events.
  void ParseMetadata();

  std::map<TraceEvent::ProcessThreadID, std::string> thread_names_;
  std::vector<TraceEvent> raw_events_;
  bool allow_assocation_changes_;

  DISALLOW_COPY_AND_ASSIGN(TraceAnalyzer);
};

}  // namespace trace_analyzer

#endif  // BASE_TEST_TRACE_EVENT_ANALYZER_H_
