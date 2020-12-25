//
// Copyright Aliaksei Levin (levlam@telegram.org), Arseny Smirnov (arseny30@gmail.com) 2014-2020
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include "td/telegram/DialogId.h"
#include "td/telegram/GroupCallId.h"
#include "td/telegram/GroupCallParticipant.h"
#include "td/telegram/InputGroupCallId.h"
#include "td/telegram/td_api.h"
#include "td/telegram/telegram_api.h"
#include "td/telegram/UserId.h"

#include "td/actor/actor.h"
#include "td/actor/PromiseFuture.h"
#include "td/actor/Timeout.h"

#include "td/utils/Status.h"

#include <unordered_map>

namespace td {

class Td;

class GroupCallManager : public Actor {
 public:
  GroupCallManager(Td *td, ActorShared<> parent);
  GroupCallManager(const GroupCallManager &) = delete;
  GroupCallManager &operator=(const GroupCallManager &) = delete;
  GroupCallManager(GroupCallManager &&) = delete;
  GroupCallManager &operator=(GroupCallManager &&) = delete;
  ~GroupCallManager() override;

  GroupCallId get_group_call_id(InputGroupCallId input_group_call_id, DialogId dialog_id);

  void create_voice_chat(DialogId dialog_id, Promise<GroupCallId> &&promise);

  void get_group_call(GroupCallId group_call_id, Promise<td_api::object_ptr<td_api::groupCall>> &&promise);

  void reload_group_call(InputGroupCallId input_group_call_id,
                         Promise<td_api::object_ptr<td_api::groupCall>> &&promise);

  void join_group_call(GroupCallId group_call_id, td_api::object_ptr<td_api::groupCallPayload> &&payload, int32 source,
                       bool is_muted, Promise<td_api::object_ptr<td_api::groupCallJoinResponse>> &&promise);

  void toggle_group_call_mute_new_participants(GroupCallId group_call_id, bool mute_new_participants,
                                               Promise<Unit> &&promise);

  void invite_group_call_participants(GroupCallId group_call_id, vector<UserId> &&user_ids, Promise<Unit> &&promise);

  void set_group_call_participant_is_speaking(GroupCallId group_call_id, int32 source, bool is_speaking,
                                              Promise<Unit> &&promise, int32 date = 0);

  void toggle_group_call_participant_is_muted(GroupCallId group_call_id, UserId user_id, bool is_muted,
                                              Promise<Unit> &&promise);

  void check_group_call_is_joined(GroupCallId group_call_id, Promise<Unit> &&promise);

  void load_group_call_participants(GroupCallId group_call_id, int32 limit, Promise<Unit> &&promise);

  void leave_group_call(GroupCallId group_call_id, Promise<Unit> &&promise);

  void discard_group_call(GroupCallId group_call_id, Promise<Unit> &&promise);

  void on_update_group_call(tl_object_ptr<telegram_api::GroupCall> group_call_ptr, DialogId dialog_id);

  void on_user_speaking_in_group_call(GroupCallId group_call_id, UserId user_id, int32 date, bool recursive = false);

  void on_get_group_call_participants(InputGroupCallId input_group_call_id,
                                      tl_object_ptr<telegram_api::phone_groupParticipants> &&participants, bool is_load,
                                      const string &offset);

  void on_update_group_call_participants(InputGroupCallId input_group_call_id,
                                         vector<tl_object_ptr<telegram_api::groupCallParticipant>> &&participants,
                                         int32 version);

  void process_join_group_call_response(InputGroupCallId input_group_call_id, uint64 generation,
                                        tl_object_ptr<telegram_api::Updates> &&updates, Promise<Unit> &&promise);

 private:
  struct GroupCall;
  struct GroupCallParticipants;
  struct GroupCallRecentSpeakers;
  struct PendingJoinRequest;

  static constexpr int32 RECENT_SPEAKER_TIMEOUT = 5 * 60;

  void tear_down() override;

  static void on_pending_send_speaking_action_timeout_callback(void *group_call_manager_ptr, int64 group_call_id_int);

  void on_send_speaking_action_timeout(GroupCallId group_call_id);

  static void on_recent_speaker_update_timeout_callback(void *group_call_manager_ptr, int64 group_call_id_int);

  void on_recent_speaker_update_timeout(GroupCallId group_call_id);

  static void on_sync_participants_timeout_callback(void *group_call_manager_ptr, int64 group_call_id_int);

  void on_sync_participants_timeout(GroupCallId group_call_id);

  Result<InputGroupCallId> get_input_group_call_id(GroupCallId group_call_id);

  GroupCallId get_next_group_call_id(InputGroupCallId input_group_call_id);

  GroupCall *add_group_call(InputGroupCallId input_group_call_id, DialogId dialog_id);

  const GroupCall *get_group_call(InputGroupCallId input_group_call_id) const;
  GroupCall *get_group_call(InputGroupCallId input_group_call_id);

  Status can_manage_group_calls(DialogId dialog_id);

  void on_voice_chat_created(DialogId dialog_id, InputGroupCallId input_group_call_id, Promise<GroupCallId> &&promise);

  void finish_get_group_call(InputGroupCallId input_group_call_id,
                             Result<tl_object_ptr<telegram_api::phone_groupCall>> &&result);

  bool need_group_call_participants(InputGroupCallId input_group_call_id) const;

  bool process_pending_group_call_participant_updates(InputGroupCallId input_group_call_id);

  void sync_group_call_participants(InputGroupCallId input_group_call_id);

  void on_sync_group_call_participants_failed(InputGroupCallId input_group_call_id);

  void process_group_call_participants(InputGroupCallId group_call_id,
                                       vector<tl_object_ptr<telegram_api::groupCallParticipant>> &&participants,
                                       bool is_load, bool is_sync);

  int process_group_call_participant(InputGroupCallId group_call_id, GroupCallParticipant &&participant);

  bool on_join_group_call_response(InputGroupCallId input_group_call_id, string json_response);

  void finish_join_group_call(InputGroupCallId input_group_call_id, uint64 generation, Status error);

  void on_group_call_left(InputGroupCallId input_group_call_id, int32 source);

  void on_group_call_left_impl(GroupCall *group_call);

  InputGroupCallId update_group_call(const tl_object_ptr<telegram_api::GroupCall> &group_call_ptr, DialogId dialog_id);

  void on_receive_group_call_version(InputGroupCallId input_group_call_id, int32 version);

  void on_participant_speaking_in_group_call(InputGroupCallId input_group_call_id,
                                             const GroupCallParticipant &participant);

  void remove_recent_group_call_speaker(InputGroupCallId input_group_call_id, UserId user_id);

  void on_group_call_recent_speakers_updated(const GroupCall *group_call, GroupCallRecentSpeakers *recent_speakers);

  UserId set_group_call_participant_is_speaking_by_source(InputGroupCallId input_group_call_id, int32 source,
                                                          bool is_speaking, int32 date);

  static Result<td_api::object_ptr<td_api::groupCallJoinResponse>> get_group_call_join_response_object(
      string json_response);

  void try_clear_group_call_participants(InputGroupCallId input_group_call_id);

  void update_group_call_dialog(const GroupCall *group_call, const char *source);

  vector<int32> get_recent_speaker_user_ids(const GroupCall *group_call, bool for_update);

  tl_object_ptr<td_api::updateGroupCall> get_update_group_call_object(const GroupCall *group_call,
                                                                      vector<int32> recent_speaker_user_ids) const;

  tl_object_ptr<td_api::groupCall> get_group_call_object(const GroupCall *group_call,
                                                         vector<int32> recent_speaker_user_ids) const;

  tl_object_ptr<td_api::updateGroupCallParticipant> get_update_group_call_participant_object(
      GroupCallId group_call_id, const GroupCallParticipant &participant);

  void send_update_group_call(const GroupCall *group_call, const char *source);

  void send_update_group_call_participant(GroupCallId group_call_id, const GroupCallParticipant &participant);

  void send_update_group_call_participant(InputGroupCallId input_group_call_id,
                                          const GroupCallParticipant &participant);

  Td *td_;
  ActorShared<> parent_;

  GroupCallId max_group_call_id_;

  vector<InputGroupCallId> input_group_call_ids_;

  std::unordered_map<InputGroupCallId, unique_ptr<GroupCall>, InputGroupCallIdHash> group_calls_;

  std::unordered_map<InputGroupCallId, unique_ptr<GroupCallParticipants>, InputGroupCallIdHash>
      group_call_participants_;

  std::unordered_map<GroupCallId, unique_ptr<GroupCallRecentSpeakers>, GroupCallIdHash> group_call_recent_speakers_;

  std::unordered_map<InputGroupCallId, vector<Promise<td_api::object_ptr<td_api::groupCall>>>, InputGroupCallIdHash>
      load_group_call_queries_;

  std::unordered_map<InputGroupCallId, unique_ptr<PendingJoinRequest>, InputGroupCallIdHash> pending_join_requests_;
  uint64 join_group_request_generation_ = 0;

  MultiTimeout pending_send_speaking_action_timeout_{"PendingSendSpeakingActionTimeout"};
  MultiTimeout recent_speaker_update_timeout_{"RecentSpeakerUpdateTimeout"};
  MultiTimeout sync_participants_timeout_{"SyncParticipantsTimeout"};
};

}  // namespace td
