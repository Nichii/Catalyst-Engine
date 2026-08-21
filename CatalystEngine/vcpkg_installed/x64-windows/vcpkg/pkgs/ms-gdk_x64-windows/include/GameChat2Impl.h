#pragma once

#include <GameChat2_c.h>

#ifndef __cplusplus
#error "This header requires C++"
#endif // end if not defined __cplusplus

namespace xbox { namespace services {
namespace game_chat_2 {

//
// C to C++ structure and constant verification
//
#pragma push_macro("C_ASSERT")
#undef C_ASSERT
#define C_ASSERT(e) typedef char __C_ASSERT__[(e)?1:-1]

C_ASSERT(GC_MAX_CHAT_TEXT_MESSAGE_LENGTH == c_maxChatTextMessageLength);
C_ASSERT(GC_MAX_XBOX_USER_ID_LENGTH == c_maxXboxUserIdLength);
C_ASSERT(GC_ANY_PROCESSOR_NUMBER == c_anyProcessorNumber);
C_ASSERT(sizeof(CHAT_USER_HANDLE) == sizeof(chat_user*));
C_ASSERT(sizeof(int64_t) == sizeof(game_chat_state_change_type)); // GC_STATE_CHANGE_TYPE
C_ASSERT(sizeof(GC_TRANSPORT_REQUIREMENT) == sizeof(game_chat_data_transport_requirement));
C_ASSERT(sizeof(GC_CHAT_USER_CHAT_INDICATOR) == sizeof(game_chat_user_chat_indicator));
C_ASSERT(sizeof(GC_THREAD_ID) == sizeof(game_chat_thread_id));
C_ASSERT(sizeof(GC_AUDIO_ENCODING_BITRATE) == sizeof(game_chat_audio_encoding_bitrate));
C_ASSERT(sizeof(GC_SPEECH_TO_TEXT_CONVERSION_MODE) == sizeof(game_chat_speech_to_text_conversion_mode));
C_ASSERT(sizeof(GC_SHARED_DEVICE_COMMUNICATION_RELATIONSHIP_RESOLUTION_MODE) == sizeof(game_chat_shared_device_communication_relationship_resolution_mode));
C_ASSERT(sizeof(GC_COMMUNICATION_RELATIONSHIP) == sizeof(game_chat_communication_relationship_flags));
C_ASSERT(sizeof(GC_COMMUNICATION_RELATIONSHIP_ADJUSTER) == sizeof(game_chat_communication_relationship_adjuster));
C_ASSERT(sizeof(GC_STATE_CHANGE) == sizeof(game_chat_state_change));
C_ASSERT(sizeof(GC_CHAT_TEXT_RECEIVED_STATE_CHANGE) == sizeof(game_chat_text_chat_received_state_change));
C_ASSERT(sizeof(GC_TRANSCRIBED_CHAT_RECEIVED_STATE_CHANGE) == sizeof(game_chat_transcribed_chat_received_state_change));
C_ASSERT(sizeof(GC_COMMUNICATION_RELATIONSHIP_ADJUSTER_CHANGED_STATE_CHANGE) == sizeof(game_chat_communication_relationship_adjuster_changed_state_change));
C_ASSERT(sizeof(GC_DATA_FRAME) == sizeof(game_chat_data_frame));
C_ASSERT(sizeof(GC_MEM_ALLOC_FUNC) == sizeof(game_chat_allocate_memory_callback));
C_ASSERT(sizeof(GC_MEM_FREE_FUNC) == sizeof(game_chat_free_memory_callback));
C_ASSERT(sizeof(GC_MEMORY_TYPE) == sizeof(uint32_t));
C_ASSERT(sizeof(GC_SAMPLE_TYPE) == sizeof(game_chat_sample_type));
C_ASSERT(sizeof(GC_AUDIO_FORMAT) == sizeof(game_chat_audio_format));
C_ASSERT(sizeof(GC_STREAM_STATE_CHANGE) == sizeof(game_chat_stream_state_change));

C_ASSERT(static_cast<int32_t>(GC_STATE_CHANGE_TYPE_CHAT_TEXT_RECEIVED) == static_cast<int32_t>(game_chat_state_change_type::text_chat_received));
C_ASSERT(static_cast<int32_t>(GC_STATE_CHANGE_TYPE_TRANSCRIBED_CHAT_RECEIVED) == static_cast<int32_t>(game_chat_state_change_type::transcribed_chat_received));
C_ASSERT(static_cast<int32_t>(GC_STATE_CHANGE_TYPE_CHAT_TEXT_CONVERSION_PREFERENCE_CHANGED) == static_cast<int32_t>(game_chat_state_change_type::text_conversion_preference_changed));
C_ASSERT(static_cast<int32_t>(GC_STATE_CHANGE_TYPE_COMMUNICATION_RELATIONSHIP_ADJUSTER_CHANGED) == static_cast<int32_t>(game_chat_state_change_type::communication_relationship_adjuster_changed));
C_ASSERT(static_cast<int32_t>(GC_TRANSPORT_REQUIREMENT_BEST_EFFORT) == static_cast<int32_t>(game_chat_data_transport_requirement::best_effort));

#pragma pop_macro("C_ASSERT")

#ifndef GAMECHAT_INFORM_IF_FAILED
#define GAMECHAT_INFORM_IF_FAILED(hrGameChatResult)                       \
    do {                                                                  \
        HRESULT gameChatErrorCode = (hrGameChatResult);                   \
        if (FAILED(gameChatErrorCode))                                    \
        {                                                                 \
            GameChatFailFastWithInform(gameChatErrorCode, __FUNCTIONW__); \
        }                                                                 \
    } while(0)
#endif // GAMECHAT_INFORM_IF_FAILED

//
// chat_manager class implementation
//

chat_manager::chat_manager(
    )
{
}

chat_manager::~chat_manager(
    )
{
}

chat_manager&
chat_manager::singleton_instance(
    )
{
    static chat_manager chatManagerSingleton;
    return chatManagerSingleton;
}

void
chat_manager::set_memory_callbacks(
    _In_opt_ game_chat_allocate_memory_callback allocateMemoryCallback,
    _In_opt_ game_chat_free_memory_callback freeMemoryCallback
    ) noexcept
{
    GAMECHAT_INFORM_IF_FAILED(ChatManagerSetMemFunctions(allocateMemoryCallback, freeMemoryCallback));
}

void
chat_manager::get_memory_callbacks(
    _Out_ game_chat_allocate_memory_callback * allocateMemoryCallback,
    _Out_ game_chat_free_memory_callback * freeMemoryCallback
    ) noexcept
{
    GAMECHAT_INFORM_IF_FAILED(ChatManagerGetMemFunctions(allocateMemoryCallback, freeMemoryCallback));
}

void
chat_manager::set_thread_processor(
    _In_ game_chat_thread_id threadId,
    _In_ uint32_t processorNumber
    ) noexcept
{
    GAMECHAT_INFORM_IF_FAILED(ChatManagerSetThreadProcessor(static_cast<GC_THREAD_ID>(threadId), processorNumber));
}

void
chat_manager::get_thread_processor(
    _In_ game_chat_thread_id threadId,
    _Out_ uint32_t * processorNumber
    ) noexcept
{
    GAMECHAT_INFORM_IF_FAILED(ChatManagerGetThreadProcessor(static_cast<GC_THREAD_ID>(threadId), processorNumber));
}

void
chat_manager::set_thread_affinity_mask(
    _In_ game_chat_thread_id threadId,
    _In_ uint64_t affinityMask
    ) noexcept
{
    GAMECHAT_INFORM_IF_FAILED(ChatManagerSetThreadAffinityMask(static_cast<GC_THREAD_ID>(threadId), affinityMask));
}

void
chat_manager::get_thread_affinity_mask(
    _In_ game_chat_thread_id threadId,
    _Out_ uint64_t* affinityMask
    ) noexcept
{
    GAMECHAT_INFORM_IF_FAILED(ChatManagerGetThreadAffinityMask(static_cast<GC_THREAD_ID>(threadId), affinityMask));
}

void
chat_manager::set_legacy_era_uwp_compat_mode_enabled(
    _In_ bool enabled
    ) noexcept
{
    GAMECHAT_INFORM_IF_FAILED(ChatManagerSetLegacyUwpEraCompatModeEnabled(enabled));
}

void
chat_manager::initialize(
    _In_ uint32_t maxChatUserCount,
    _In_range_(0, 1) float defaultAudioRenderVolume,
    _In_ game_chat_communication_relationship_flags defaultLocalToRemoteCommunicationRelationship,
    _In_ game_chat_shared_device_communication_relationship_resolution_mode sharedDeviceResolutionMode,
    _In_ game_chat_speech_to_text_conversion_mode speechToTextConversionMode,
    _In_ game_chat_audio_manipulation_mode_flags audioManipulationMode
    ) noexcept
{
    GAMECHAT_INFORM_IF_FAILED(ChatManagerInitialize(
        maxChatUserCount,
        defaultAudioRenderVolume,
        static_cast<GC_COMMUNICATION_RELATIONSHIP>(defaultLocalToRemoteCommunicationRelationship),
        static_cast<GC_SHARED_DEVICE_COMMUNICATION_RELATIONSHIP_RESOLUTION_MODE>(sharedDeviceResolutionMode),
        static_cast<GC_SPEECH_TO_TEXT_CONVERSION_MODE>(speechToTextConversionMode),
        static_cast<GC_AUDIO_MANIPULATION_MODE>(audioManipulationMode)));
}

void
chat_manager::cleanup(
    ) noexcept
{
    GAMECHAT_INFORM_IF_FAILED(ChatManagerCleanup());
}

void
chat_manager::set_audio_encoding_bitrate(
    _In_ game_chat_audio_encoding_bitrate audioEncodingBiterate
    ) noexcept
{
    GAMECHAT_INFORM_IF_FAILED(ChatManagerSetAudioEncodingBitrate(
        static_cast<GC_AUDIO_ENCODING_BITRATE>(audioEncodingBiterate)));
}

game_chat_audio_encoding_bitrate
chat_manager::audio_encoding_bitrate() const noexcept
{
    GC_AUDIO_ENCODING_BITRATE audioEncodingBiterate;
    GAMECHAT_INFORM_IF_FAILED(ChatManagerGetAudioEncodingBitrate(&audioEncodingBiterate));
    return static_cast<game_chat_audio_encoding_bitrate>(audioEncodingBiterate);
}

void
chat_manager::start_processing_data_frames(
    _Out_ uint32_t * dataFrameCount,
    _Outptr_result_buffer_(*dataFrameCount) game_chat_data_frame_array * dataFrames
    ) noexcept
{
    GAMECHAT_INFORM_IF_FAILED(ChatManagerStartProcessingDataFrames(
        dataFrameCount,
        reinterpret_cast<const GC_DATA_FRAME * const **>(dataFrames)));
}

void
chat_manager::finish_processing_data_frames(
    _In_ game_chat_data_frame_array dataFrames
    ) noexcept
{
    GAMECHAT_INFORM_IF_FAILED(ChatManagerFinishProcessingDataFrames(
        reinterpret_cast<const GC_DATA_FRAME * const *>(dataFrames)));
}

void
chat_manager::start_processing_state_changes(
    _Out_ uint32_t * stateChangeCount,
    _Outptr_result_buffer_(*stateChangeCount) game_chat_state_change_array * stateChanges
    ) noexcept
{
    GAMECHAT_INFORM_IF_FAILED(ChatManagerStartProcessingStateChanges(
        stateChangeCount,
        reinterpret_cast<const GC_STATE_CHANGE * const **>(stateChanges)));
}

void
chat_manager::finish_processing_state_changes(
    _In_ game_chat_state_change_array stateChanges
    ) noexcept
{
    GAMECHAT_INFORM_IF_FAILED(ChatManagerFinishProcessingStateChanges(
        reinterpret_cast<const GC_STATE_CHANGE * const *>(stateChanges)));
}

chat_user *
chat_manager::add_local_user(
    _In_ PCWSTR xboxUserId
    ) noexcept
{
    CHAT_USER_HANDLE userHandle;
    GAMECHAT_INFORM_IF_FAILED(ChatManagerAddLocalUser(xboxUserId, &userHandle));
    return (chat_user*)userHandle;
}

chat_user *
chat_manager::add_remote_user(
    _In_ PCWSTR xboxUserId,
    _In_ uint64_t remoteConsoleIdentifier
    ) noexcept
{
    CHAT_USER_HANDLE userHandle;
    GAMECHAT_INFORM_IF_FAILED(ChatManagerAddRemoteUser(xboxUserId, remoteConsoleIdentifier, &userHandle));
    return (chat_user*)userHandle;
}

void
chat_manager::remove_user(
    _In_ _Post_invalid_ chat_user * chatUser
    ) noexcept
{
    GAMECHAT_INFORM_IF_FAILED(ChatManagerRemoveUser(reinterpret_cast<CHAT_USER_HANDLE>(chatUser)));
}

void
chat_manager::process_incoming_data(
    _In_ uint64_t senderConsoleIdentifier,
    _In_ uint32_t bufferByteCount,
    _In_reads_(bufferByteCount) const void * buffer
    ) noexcept
{
    GAMECHAT_INFORM_IF_FAILED(ChatManagerProcessIncomingPacket(senderConsoleIdentifier, bufferByteCount, buffer));
}

game_chat_audio_manipulation_mode_flags
chat_manager::audio_manipulation_mode(
    ) noexcept
{
    GC_AUDIO_MANIPULATION_MODE audioManipulationMode;
    GAMECHAT_INFORM_IF_FAILED(ChatManagerGetAudioManipulationMode(&audioManipulationMode));
    return static_cast<game_chat_audio_manipulation_mode_flags>(audioManipulationMode);
}

void
chat_manager::get_chat_users(
    _Out_ uint32_t * chatUserCount,
    _Outptr_result_buffer_(*chatUserCount) chat_user_array * chatUsers
    ) const noexcept
{
    GAMECHAT_INFORM_IF_FAILED(ChatManagerGetChatUsers(chatUserCount, (const CHAT_USER_HANDLE **)chatUsers));
}

void
chat_manager::get_pre_encode_audio_streams(
    _Out_ uint32_t* preEncodeAudioStreamCount,
    _Outptr_result_buffer_(*preEncodeAudioStreamCount) pre_encode_audio_stream_array* preEncodeAudioStreams
    ) noexcept
{
    GAMECHAT_INFORM_IF_FAILED(ChatManagerGetPreEncodeAudioStreams(
        preEncodeAudioStreamCount,
        reinterpret_cast<PRE_ENCODE_AUDIO_STREAM_ARRAY*>(preEncodeAudioStreams)));
}

void
chat_manager::get_post_decode_audio_source_streams(
    _Out_ uint32_t* postDecodeAudioSourceStreamCount,
    _Outptr_result_buffer_(*postDecodeAudioSourceStreamCount) post_decode_audio_source_stream_array* postDecodeAudioSourceStreams
    ) noexcept
{
    GAMECHAT_INFORM_IF_FAILED(ChatManagerGetPostDecodeAudioSourceStreams(
        postDecodeAudioSourceStreamCount,
        reinterpret_cast<POST_DECODE_AUDIO_SOURCE_STREAM_ARRAY*>(postDecodeAudioSourceStreams)));
}

void
chat_manager::get_post_decode_audio_sink_streams(
    _Out_ uint32_t* postDecodeAudioSinkStreamCount,
    _Outptr_result_buffer_(*postDecodeAudioSinkStreamCount) post_decode_audio_sink_stream_array* postDecodeAudioSinkStreams
    ) noexcept
{
    GAMECHAT_INFORM_IF_FAILED(ChatManagerGetPostDecodeAudioSinkStreams(
        postDecodeAudioSinkStreamCount,
        reinterpret_cast<POST_DECODE_AUDIO_SINK_STREAM_ARRAY*>(postDecodeAudioSinkStreams)));
}

void
chat_manager::start_processing_stream_state_changes(
    _Out_ uint32_t* streamStateChangeCount,
    _Outptr_result_buffer_(*streamStateChangeCount) game_chat_stream_state_change_array* streamStateChanges
    ) noexcept
{
    GAMECHAT_INFORM_IF_FAILED(ChatManagerStartProcessingStreamStateChanges(
        streamStateChangeCount,
        reinterpret_cast<GC_STREAM_STATE_CHANGE_ARRAY*>(streamStateChanges)));
}

void
chat_manager::finish_processing_stream_state_changes(
    _In_ game_chat_stream_state_change_array streamStateChanges
    ) noexcept
{
    GAMECHAT_INFORM_IF_FAILED(ChatManagerFinishProcessingStreamStateChanges(
        reinterpret_cast<GC_STREAM_STATE_CHANGE_ARRAY>(streamStateChanges)));
}

//
// chat_user::chat_user_local implementation
//

void
chat_user::chat_user_local::set_communication_relationship(
    _In_ chat_user* targetUser,
    _In_ game_chat_communication_relationship_flags communicationRelationship
    ) noexcept
{
    GAMECHAT_INFORM_IF_FAILED(ChatUserSetCommunicationRelationship(
        reinterpret_cast<CHAT_USER_HANDLE>(this),
        reinterpret_cast<CHAT_USER_HANDLE>(targetUser),
        static_cast<GC_COMMUNICATION_RELATIONSHIP>(communicationRelationship)));
}

void
chat_user::chat_user_local::get_effective_communication_relationship(
    _In_ chat_user* targetUser,
    _Out_ game_chat_communication_relationship_flags * effectiveCommunicationRelationship,
    _Out_ game_chat_communication_relationship_adjuster * communicationRelationshipAdjuster
    ) const noexcept
{
    GC_COMMUNICATION_RELATIONSHIP gcCommunicationRelationship;
    GC_COMMUNICATION_RELATIONSHIP_ADJUSTER gcCommunicationRelationshipAdjuster;
    GAMECHAT_INFORM_IF_FAILED(ChatUserGetEffectiveCommunicationRelationship(
        reinterpret_cast<CHAT_USER_HANDLE>(this),
        reinterpret_cast<CHAT_USER_HANDLE>(targetUser),
        &gcCommunicationRelationship,
        &gcCommunicationRelationshipAdjuster));
    *effectiveCommunicationRelationship = static_cast<game_chat_communication_relationship_flags>(
        gcCommunicationRelationship);
    *communicationRelationshipAdjuster = static_cast<game_chat_communication_relationship_adjuster>(
        gcCommunicationRelationshipAdjuster);
}

_Ret_range_(0, 1)
float
chat_user::chat_user_local::audio_render_volume(
    _In_ chat_user * targetUser
    ) const noexcept
{
    float volume;
    GAMECHAT_INFORM_IF_FAILED(ChatUserGetAudioRenderVolume(
        reinterpret_cast<CHAT_USER_HANDLE>(this),
        reinterpret_cast<CHAT_USER_HANDLE>(targetUser),
        &volume));
    return volume;
}

void
chat_user::chat_user_local::set_audio_render_volume(
    _In_ chat_user * targetUser,
    _In_range_(0, 1) float volume
    ) noexcept
{
    GAMECHAT_INFORM_IF_FAILED(ChatUserSetAudioRenderVolume(
        reinterpret_cast<CHAT_USER_HANDLE>(this),
        reinterpret_cast<CHAT_USER_HANDLE>(targetUser),
        volume));
}

bool chat_user::chat_user_local::microphone_muted(
    ) const noexcept
{
    bool muted;
    GAMECHAT_INFORM_IF_FAILED(ChatUserGetMicrophoneMuted(reinterpret_cast<CHAT_USER_HANDLE>(this), &muted));
    return muted;
}

void
chat_user::chat_user_local::set_microphone_muted(
    _In_ bool muted
    ) noexcept
{
    GAMECHAT_INFORM_IF_FAILED(ChatUserSetMicrophoneMuted(reinterpret_cast<CHAT_USER_HANDLE>(this), muted));
}

bool
chat_user::chat_user_local::remote_user_muted(
    _In_ chat_user * targetUser
    ) noexcept
{
    bool muted;
    GAMECHAT_INFORM_IF_FAILED(ChatUserGetRemoteUserMuted(
        reinterpret_cast<CHAT_USER_HANDLE>(this),
        reinterpret_cast<CHAT_USER_HANDLE>(targetUser),
        &muted));
    return muted;
}

void
chat_user::chat_user_local::set_remote_user_muted(
    _In_ chat_user * targetUser,
    _In_ bool muted
    ) noexcept
{
    GAMECHAT_INFORM_IF_FAILED(ChatUserSetRemoteUserMuted(
        reinterpret_cast<CHAT_USER_HANDLE>(this),
        reinterpret_cast<CHAT_USER_HANDLE>(targetUser),
        muted));
}

void
chat_user::chat_user_local::send_chat_text(
    _In_ PCWSTR message
    ) noexcept
{
    GAMECHAT_INFORM_IF_FAILED(ChatUserSendChatText(reinterpret_cast<CHAT_USER_HANDLE>(this), message));
}

void
chat_user::chat_user_local::synthesize_text_to_speech(
    _In_ PCWSTR message
    ) noexcept
{
    GAMECHAT_INFORM_IF_FAILED(ChatUserSynthesizeTextToSpeech(reinterpret_cast<CHAT_USER_HANDLE>(this), message));
}

bool
chat_user::chat_user_local::speech_to_text_conversion_preference_enabled() const noexcept
{
    bool speechToTextConversionPreferenceEnabled;
    GAMECHAT_INFORM_IF_FAILED(ChatUserGetSpeechToTextConversionPreferenceEnabled(
        reinterpret_cast<CHAT_USER_HANDLE>(this),
        &speechToTextConversionPreferenceEnabled));
    return speechToTextConversionPreferenceEnabled;
}

bool
chat_user::chat_user_local::text_to_speech_conversion_preference_enabled() const noexcept
{
    bool textToSpeechConversionPreferenceEnabled;
    GAMECHAT_INFORM_IF_FAILED(ChatUserGetTextToSpeechConversionPreferenceEnabled(
        reinterpret_cast<CHAT_USER_HANDLE>(this),
        &textToSpeechConversionPreferenceEnabled));
    return textToSpeechConversionPreferenceEnabled;
}

//
// chat_user implementation
//

_Ret_z_
PCWSTR
chat_user::xbox_user_id(
    ) const noexcept
{
    PCWSTR xboxUserId;
    GAMECHAT_INFORM_IF_FAILED(ChatUserGetXboxUserId(reinterpret_cast<CHAT_USER_HANDLE>(this), &xboxUserId));
    return xboxUserId;
}

_Ret_maybenull_
chat_user::chat_user_local *
chat_user::local(
    ) noexcept
{
    bool isLocal;
    GAMECHAT_INFORM_IF_FAILED(ChatUserGetIsLocal(reinterpret_cast<CHAT_USER_HANDLE>(this), &isLocal));
    return isLocal ? reinterpret_cast<chat_user_local *>(this) : nullptr;
}

_Ret_maybenull_
const chat_user::chat_user_local *
chat_user::local(
    ) const noexcept
{
    bool isLocal;
    GAMECHAT_INFORM_IF_FAILED(ChatUserGetIsLocal(reinterpret_cast<CHAT_USER_HANDLE>(this), &isLocal));
    return isLocal ? reinterpret_cast<const chat_user_local *>(this) : nullptr;
}

void
chat_user::set_custom_user_context(
    _In_opt_ void * customUserContext
    ) noexcept
{
    GAMECHAT_INFORM_IF_FAILED(ChatUserSetCustomUserContext(
        reinterpret_cast<CHAT_USER_HANDLE>(this),
        customUserContext));
}

_Ret_maybenull_
void *
chat_user::custom_user_context(
    ) const noexcept
{
    void * customUserContext;
    GAMECHAT_INFORM_IF_FAILED(ChatUserGetCustomUserContext(
        reinterpret_cast<CHAT_USER_HANDLE>(this),
        &customUserContext));
    return customUserContext;
}

game_chat_user_chat_indicator
chat_user::chat_indicator() const noexcept
{
    GC_CHAT_USER_CHAT_INDICATOR chatIndicator;
    GAMECHAT_INFORM_IF_FAILED(ChatUserGetChatIndicator(reinterpret_cast<CHAT_USER_HANDLE>(this), &chatIndicator));
    return static_cast<game_chat_user_chat_indicator>(chatIndicator);
}

//
// pre_encode_audio_stream implementation
//

void
pre_encode_audio_stream::get_users(
    _Out_ uint32_t* chatUserCount,
    _Outptr_result_buffer_(*chatUserCount) chat_user_array* chatUsers
    ) const noexcept
{
    GAMECHAT_INFORM_IF_FAILED(PreEncodeAudioStreamGetUsers(
        reinterpret_cast<PRE_ENCODE_AUDIO_STREAM_HANDLE>(this),
        chatUserCount,
        (CHAT_USER_ARRAY*)(chatUsers)));
}

game_chat_audio_format
pre_encode_audio_stream::get_pre_processed_format(
    ) const noexcept
{
    game_chat_audio_format format;
    GAMECHAT_INFORM_IF_FAILED(PreEncodeAudioStreamGetPreprocessedFormat(
        reinterpret_cast<PRE_ENCODE_AUDIO_STREAM_HANDLE>(this),
        reinterpret_cast<GC_AUDIO_FORMAT*>(&format)));
    return format;
}

void
pre_encode_audio_stream::set_processed_format(
    _In_ game_chat_audio_format format
    ) noexcept
{
    GAMECHAT_INFORM_IF_FAILED(PreEncodeAudioStreamSetProcessedFormat(
        reinterpret_cast<PRE_ENCODE_AUDIO_STREAM_HANDLE>(this),
        *reinterpret_cast<GC_AUDIO_FORMAT*>(&format)));
}

uint32_t
pre_encode_audio_stream::get_available_buffer_count(
    ) const noexcept
{
    uint32_t count;
    GAMECHAT_INFORM_IF_FAILED(PreEncodeAudioStreamGetAvailableBufferCount(
        reinterpret_cast<PRE_ENCODE_AUDIO_STREAM_HANDLE>(this),
        &count));
    return count;
}

void
pre_encode_audio_stream::get_next_buffer(
    _Out_ uint32_t* byteCount,
    _Outptr_result_bytebuffer_maybenull_(*byteCount) void** buffer
    ) noexcept
{
    GAMECHAT_INFORM_IF_FAILED(PreEncodeAudioStreamGetNextBuffer(
        reinterpret_cast<PRE_ENCODE_AUDIO_STREAM_HANDLE>(this),
        byteCount,
        buffer));
}

void
pre_encode_audio_stream::return_buffer(
    _In_ void* buffer
    ) noexcept
{
    GAMECHAT_INFORM_IF_FAILED(PreEncodeAudioStreamReturnBuffer(
        reinterpret_cast<PRE_ENCODE_AUDIO_STREAM_HANDLE>(this),
        buffer));
}

void
pre_encode_audio_stream::submit_buffer(
    _In_ uint32_t byteCount,
    _In_reads_bytes_(byteCount) void* buffer
    ) noexcept
{
    GAMECHAT_INFORM_IF_FAILED(PreEncodeAudioStreamSubmitBuffer(
        reinterpret_cast<PRE_ENCODE_AUDIO_STREAM_HANDLE>(this),
        byteCount,
        buffer));
}

void
pre_encode_audio_stream::set_custom_stream_context(
    _In_ void* customStreamContext
    ) noexcept
{
    GAMECHAT_INFORM_IF_FAILED(PreEncodeAudioStreamSetCustomStreamContext(
        reinterpret_cast<PRE_ENCODE_AUDIO_STREAM_HANDLE>(this),
        customStreamContext));
}

void*
pre_encode_audio_stream::custom_stream_context(
    ) const noexcept
{
    void* customStreamContext;
    GAMECHAT_INFORM_IF_FAILED(PreEncodeAudioStreamGetCustomStreamContext(
        reinterpret_cast<PRE_ENCODE_AUDIO_STREAM_HANDLE>(this),
        &customStreamContext));
    return customStreamContext;
}

bool
pre_encode_audio_stream::is_open(
    ) const noexcept
{
    bool isOpen;
    GAMECHAT_INFORM_IF_FAILED(PreEncodeAudioStreamIsOpen(
        reinterpret_cast<PRE_ENCODE_AUDIO_STREAM_HANDLE>(this),
        &isOpen));
    return isOpen;
}

//
// post_decode_audio_source_stream implementation
//

void
post_decode_audio_source_stream::get_users(
    _Out_ uint32_t* chatUserCount,
    _Outptr_result_buffer_(*chatUserCount) chat_user_array* chatUsers
    ) const noexcept
{
    GAMECHAT_INFORM_IF_FAILED(PostDecodeAudioSourceStreamGetUsers(
        reinterpret_cast<POST_DECODE_AUDIO_SOURCE_STREAM_HANDLE>(this),
        chatUserCount,
        (CHAT_USER_ARRAY*)(chatUsers)));
}

game_chat_audio_format
post_decode_audio_source_stream::get_pre_processed_format(
    ) const noexcept
{
    game_chat_audio_format format;
    GAMECHAT_INFORM_IF_FAILED(PostDecodeAudioSourceStreamGetPreprocessedFormat(
        reinterpret_cast<POST_DECODE_AUDIO_SOURCE_STREAM_HANDLE>(this),
        reinterpret_cast<GC_AUDIO_FORMAT*>(&format)));
    return format;
}

uint32_t
post_decode_audio_source_stream::get_available_buffer_count(
    ) const noexcept
{
    uint32_t count;
    GAMECHAT_INFORM_IF_FAILED(PostDecodeAudioSourceStreamGetAvailableBufferCount(
        reinterpret_cast<POST_DECODE_AUDIO_SOURCE_STREAM_HANDLE>(this),
        &count));
    return count;
}

void
post_decode_audio_source_stream::get_next_buffer(
    _Out_ uint32_t* byteCount,
    _Outptr_result_bytebuffer_maybenull_(*byteCount) void** buffer
    ) noexcept
{
    GAMECHAT_INFORM_IF_FAILED(PostDecodeAudioSourceStreamGetNextBuffer(
        reinterpret_cast<POST_DECODE_AUDIO_SOURCE_STREAM_HANDLE>(this),
        byteCount,
        buffer));
}

void
post_decode_audio_source_stream::return_buffer(
    _In_ void* buffer
    ) noexcept
{
    GAMECHAT_INFORM_IF_FAILED(PostDecodeAudioSourceStreamReturnBuffer(
        reinterpret_cast<POST_DECODE_AUDIO_SOURCE_STREAM_HANDLE>(this),
        buffer));
}

void
post_decode_audio_source_stream::set_custom_stream_context(
    _In_ void* customStreamContext
    ) noexcept
{
    GAMECHAT_INFORM_IF_FAILED(PostDecodeAudioSourceStreamSetCustomStreamContext(
        reinterpret_cast<POST_DECODE_AUDIO_SOURCE_STREAM_HANDLE>(this),
        customStreamContext));
}

void*
post_decode_audio_source_stream::custom_stream_context(
    ) const noexcept
{
    void* customStreamContext;
    GAMECHAT_INFORM_IF_FAILED(PostDecodeAudioSourceStreamGetCustomStreamContext(
        reinterpret_cast<POST_DECODE_AUDIO_SOURCE_STREAM_HANDLE>(this),
        &customStreamContext));
    return customStreamContext;
}

bool
post_decode_audio_source_stream::is_open(
    ) const noexcept
{
    bool isOpen;
    GAMECHAT_INFORM_IF_FAILED(PostDecodeAudioSourceStreamIsOpen(
        reinterpret_cast<POST_DECODE_AUDIO_SOURCE_STREAM_HANDLE>(this),
        &isOpen));
    return isOpen;
}

//
// post_decode_audio_sink_stream implementation
//

void
post_decode_audio_sink_stream::get_users(
    _Out_ uint32_t* chatUserCount,
    _Outptr_result_buffer_(*chatUserCount) chat_user_array* chatUsers
    ) const noexcept
{
    GAMECHAT_INFORM_IF_FAILED(PostDecodeAudioSinkStreamGetUsers(
        reinterpret_cast<POST_DECODE_AUDIO_SINK_STREAM_HANDLE>(this),
        chatUserCount,
        (CHAT_USER_ARRAY*)(chatUsers)));
}

void
post_decode_audio_sink_stream::set_processed_format(
    _In_ game_chat_audio_format format
    ) noexcept
{
    GAMECHAT_INFORM_IF_FAILED(PostDecodeAudioSinkStreamSetProcessedFormat(
        reinterpret_cast<POST_DECODE_AUDIO_SINK_STREAM_HANDLE>(this),
        *reinterpret_cast<GC_AUDIO_FORMAT*>(&format)));
}

void
post_decode_audio_sink_stream::submit_mixed_buffer(
    _In_ uint32_t byteCount,
    _In_reads_bytes_(byteCount) void* mixedBuffer
    ) noexcept
{
    GAMECHAT_INFORM_IF_FAILED(PostDecodeAudioSinkStreamSubmitMixedBuffer(
        reinterpret_cast<POST_DECODE_AUDIO_SINK_STREAM_HANDLE>(this),
        byteCount,
        mixedBuffer));
}

bool post_decode_audio_sink_stream::can_receive_audio_from_source_stream(
    _In_ post_decode_audio_source_stream* sourceStream,
    _Out_opt_ float* volume
    ) const noexcept
{
    bool canReceiveAudio;
    GAMECHAT_INFORM_IF_FAILED(PostDecodeAudioSinkStreamCanReceiveAudioFromSourceStream(
        reinterpret_cast<POST_DECODE_AUDIO_SINK_STREAM_HANDLE>(this),
        reinterpret_cast<POST_DECODE_AUDIO_SOURCE_STREAM_HANDLE>(sourceStream),
        volume,
        &canReceiveAudio));
    return canReceiveAudio;
}

PCWSTR
post_decode_audio_sink_stream::get_device_id(
    ) const noexcept
{
    PCWSTR deviceId;
    GAMECHAT_INFORM_IF_FAILED(PostDecodeAudioSinkStreamGetDeviceId(
        reinterpret_cast<POST_DECODE_AUDIO_SINK_STREAM_HANDLE>(this),
        &deviceId));
    return deviceId;
}

void
post_decode_audio_sink_stream::set_custom_stream_context(
    _In_ void* customStreamContext
    ) noexcept
{
    GAMECHAT_INFORM_IF_FAILED(PostDecodeAudioSinkStreamSetCustomStreamContext(
        reinterpret_cast<POST_DECODE_AUDIO_SINK_STREAM_HANDLE>(this),
        customStreamContext));
}

void*
post_decode_audio_sink_stream::custom_stream_context(
    ) const noexcept
{
    void* customStreamContext;
    GAMECHAT_INFORM_IF_FAILED(PostDecodeAudioSinkStreamGetCustomStreamContext(
        reinterpret_cast<POST_DECODE_AUDIO_SINK_STREAM_HANDLE>(this),
        &customStreamContext));
    return customStreamContext;
}

bool
post_decode_audio_sink_stream::is_open(
    ) const noexcept
{
    bool isOpen;
    GAMECHAT_INFORM_IF_FAILED(PostDecodeAudioSinkStreamIsOpen(
        reinterpret_cast<POST_DECODE_AUDIO_SINK_STREAM_HANDLE>(this),
        &isOpen));
    return isOpen;
}

} // end namespace game_chat_2
} } // end namespace xbox::services
