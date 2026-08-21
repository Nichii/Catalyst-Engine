#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#define GC_API __stdcall

const uint16_t GC_MAX_CHAT_TEXT_MESSAGE_LENGTH = 1023;
const uint8_t GC_MAX_XBOX_USER_ID_LENGTH = 64;
const uint32_t GC_ANY_PROCESSOR_NUMBER = 0xFFFFFFFF;
const uint64_t GC_ANY_PROCESSOR = 0xFFFFFFFFFFFFFFFF;

typedef uint32_t GC_MEMORY_TYPE;
typedef const struct CHAT_USER* CHAT_USER_HANDLE;
typedef const CHAT_USER_HANDLE* CHAT_USER_ARRAY;

typedef
_Ret_maybenull_
_Post_writable_byte_size_(size) void*
(GC_API* GC_MEM_ALLOC_FUNC)(
    _In_ size_t size,
    _In_ GC_MEMORY_TYPE memoryType
    );

typedef
void
(GC_API* GC_MEM_FREE_FUNC)(
    _In_ _Post_invalid_ void* pointer,
    _In_ GC_MEMORY_TYPE memoryType
    );

typedef enum GC_DEBUG_MEMORY_MODE
{
    GC_DEBUG_MEMORY_MODE_NONE,
    GC_DEBUG_MEMORY_MODE_PAGE_ALIGN_START,
    GC_DEBUG_MEMORY_MODE_PAGE_ALIGN_END,
} GC_DEBUG_MEMORY_MODE;

typedef enum GC_STATE_CHANGE_TYPE /* : int64_t */
{
    GC_STATE_CHANGE_TYPE_CHAT_TEXT_RECEIVED,
    GC_STATE_CHANGE_TYPE_TRANSCRIBED_CHAT_RECEIVED,
    GC_STATE_CHANGE_TYPE_CHAT_TEXT_CONVERSION_PREFERENCE_CHANGED,
    GC_STATE_CHANGE_TYPE_COMMUNICATION_RELATIONSHIP_ADJUSTER_CHANGED,
} GC_STATE_CHANGE_TYPE;

typedef enum GC_TRANSPORT_REQUIREMENT
{
    GC_TRANSPORT_REQUIREMENT_GUARANTEED,
    GC_TRANSPORT_REQUIREMENT_BEST_EFFORT,
} GC_TRANSPORT_REQUIREMENT;

typedef enum GC_CHAT_USER_CHAT_INDICATOR
{
    GC_CHAT_USER_CHAT_INDICATOR_SILENT,
    GC_CHAT_USER_CHAT_INDICATOR_TALKING,
    GC_CHAT_USER_CHAT_INDICATOR_LOCAL_MICROPHONE_MUTED,
    GC_CHAT_USER_CHAT_INDICATOR_INCOMING_COMMUNICATIONS_MUTED,
    GC_CHAT_USER_CHAT_INDICATOR_REPUTATION_RESTRICTED,
    GC_CHAT_USER_CHAT_INDICATOR_PLATFORM_RESTRICTED,
    GC_CHAT_USER_CHAT_INDICATOR_NO_CHAT_FOCUS,
    GC_CHAT_USER_CHAT_INDICATOR_NO_MICROPHONE,
} GC_CHAT_USER_CHAT_INDICATOR;

typedef enum GC_THREAD_ID
{
    GC_THREAD_ID_AUDIO,
    GC_THREAD_ID_NETWORKING,
} GC_THREAD_ID;

typedef enum GC_AUDIO_ENCODING_BITRATE
{
    GC_AUDIO_ENCODING_BITRATE_KILOBITS_PER_SECOND_12,
    GC_AUDIO_ENCODING_BITRATE_KILOBITS_PER_SECOND_16,
    GC_AUDIO_ENCODING_BITRATE_KILOBITS_PER_SECOND_24,
} GC_AUDIO_ENCODING_BITRATE;

typedef enum GC_SPEECH_TO_TEXT_CONVERSION_MODE
{
    GC_SPEECH_TO_TEXT_CONVERSION_MODE_AUTOMATIC,
    GC_SPEECH_TO_TEXT_CONVERSION_MODE_TITLE_MANAGED,
} GC_SPEECH_TO_TEXT_CONVERSION_MODE;

typedef enum GC_SHARED_DEVICE_COMMUNICATION_RELATIONSHIP_RESOLUTION_MODE
{
    GC_SHARED_DEVICE_COMMUNICATION_RELATIONSHIP_RESOLUTION_MODE_PERMISSIVE,
    GC_SHARED_DEVICE_COMMUNICATION_RELATIONSHIP_RESOLUTION_MODE_RESTRICTIVE,
} GC_SHARED_DEVICE_COMMUNICATION_RELATIONSHIP_RESOLUTION_MODE;

typedef enum GC_COMMUNICATION_RELATIONSHIP
{
    GC_COMMUNICATION_RELATIONSHIP_NONE = 0x0,
    GC_COMMUNICATION_RELATIONSHIP_SEND_MICROPHONE_AUDIO = 0x1,
    GC_COMMUNICATION_RELATIONSHIP_SEND_TEXT_TO_SPEECH_AUDIO = 0x2,
    GC_COMMUNICATION_RELATIONSHIP_SEND_TEXT = 0x4,
    GC_COMMUNICATION_RELATIONSHIP_RECEIVE_AUDIO = 0x8,
    GC_COMMUNICATION_RELATIONSHIP_RECEIVE_TEXT = 0x10,
} GC_COMMUNICATION_RELATIONSHIP;

typedef enum GC_COMMUNICATION_RELATIONSHIP_ADJUSTER
{
    GC_COMMUNICATION_RELATIONSHIP_ADJUSTER_UNINITIALIZED,
    GC_COMMUNICATION_RELATIONSHIP_ADJUSTER_INITIALIZING,
    GC_COMMUNICATION_RELATIONSHIP_ADJUSTER_NONE,
    GC_COMMUNICATION_RELATIONSHIP_ADJUSTER_PRIVILEGE,
    GC_COMMUNICATION_RELATIONSHIP_ADJUSTER_PRIVACY,
    GC_COMMUNICATION_RELATIONSHIP_ADJUSTER_REPUTATION,
    GC_COMMUNICATION_RELATIONSHIP_ADJUSTER_CONFLICT_WITH_OTHER_USER,
    GC_COMMUNICATION_RELATIONSHIP_ADJUSTER_PRIVILEGE_CHECK_FAILURE,
    GC_COMMUNICATION_RELATIONSHIP_ADJUSTER_RESOLVE_USER_ISSUE,
    GC_COMMUNICATION_RElATIONSHIP_ADJUSTER_SERVICE_UNAVAILABLE,
} GC_COMMUNICATION_RELATIONSHIP_ADJUSTER;

typedef struct GC_STATE_CHANGE
{
    int64_t /* GC_STATE_CHANGE_TYPE */ stateChangeType;
} GC_STATE_CHANGE;

typedef struct GC_CHAT_TEXT_RECEIVED_STATE_CHANGE
{
    GC_STATE_CHANGE stateChange;
    CHAT_USER_HANDLE sender;
    uint32_t receiverCount;
    _Field_size_(receiverCount) const CHAT_USER_HANDLE* receivers;
    PCWSTR message;
} GC_CHAT_TEXT_RECEIVED_STATE_CHANGE;

typedef struct GC_TRANSCRIBED_CHAT_RECEIVED_STATE_CHANGE
{
    GC_STATE_CHANGE stateChange;
    CHAT_USER_HANDLE sender;
    uint32_t receiverCount;
    _Field_size_(receiverCount) const CHAT_USER_HANDLE* receivers;
    PCWSTR message;
} GC_TRANSCRIBED_CHAT_RECEIVED_STATE_CHANGE;

typedef struct GC_CHAT_TEXT_CONVERSION_PREFERENCE_CHANGED_STATE_CHANGE
{
    GC_STATE_CHANGE stateChange;
    CHAT_USER_HANDLE chatUser;
} GC_CHAT_TEXT_CONVERSION_PREFERENCE_CHANGED_STATE_CHANGE;

typedef struct GC_COMMUNICATION_RELATIONSHIP_ADJUSTER_CHANGED_STATE_CHANGE
{
    GC_STATE_CHANGE stateChange;
    CHAT_USER_HANDLE localUser;
    CHAT_USER_HANDLE targetUser;
} GC_COMMUNICATION_RELATIONSHIP_ADJUSTER_CHANGED_STATE_CHANGE;

typedef struct GC_DATA_FRAME
{
    uint32_t targetEndpointCount;
    _Field_size_(targetEndpointCount) uint64_t* targetEndpoints;
    GC_TRANSPORT_REQUIREMENT transportRequirement;
    uint32_t packetByteCount;
    _Field_size_(packetByteCount) uint8_t* packetBuffer;
} GC_OUTGOING_PACKET_READY_STATE_CHANGE;

typedef enum GC_SAMPLE_TYPE
{
    INTEGER = 0,
    IEEE_FLOAT
} GC_SAMPLE_TYPE;

typedef struct GC_AUDIO_FORMAT
{
    uint32_t sampleRate;
    uint32_t channelMask;
    uint16_t channelCount;
    uint16_t bitsPerSample;
    GC_SAMPLE_TYPE sampleType;
    bool isInterleaved;
} GC_AUDIO_FORMAT;

typedef const struct POST_DECODE_AUDIO_SOURCE_STREAM* POST_DECODE_AUDIO_SOURCE_STREAM_HANDLE;
typedef POST_DECODE_AUDIO_SOURCE_STREAM_HANDLE * const * POST_DECODE_AUDIO_SOURCE_STREAM_ARRAY;

typedef const struct PRE_ENCODE_AUDIO_STREAM* PRE_ENCODE_AUDIO_STREAM_HANDLE;
typedef PRE_ENCODE_AUDIO_STREAM_HANDLE* const* PRE_ENCODE_AUDIO_STREAM_ARRAY;

typedef const struct POST_DECODE_AUDIO_SINK_STREAM* POST_DECODE_AUDIO_SINK_STREAM_HANDLE;
typedef POST_DECODE_AUDIO_SINK_STREAM_HANDLE * const * POST_DECODE_AUDIO_SINK_STREAM_ARRAY;

typedef enum GC_STREAM_STATE_CHANGE_TYPE /* int64_t */
{
    GC_PRE_ENCODE_AUDIO_STREAM_CREATED = 0,
    GC_PRE_ENCODE_AUDIO_STREAM_CLOSED,
    GC_PRE_ENCODE_AUDIO_STREAM_DESTROYED,
    GC_POST_DECODE_AUDIO_SOURCE_STREAM_CREATED,
    GC_POST_DECODE_AUDIO_SOURCE_STREAM_CLOSED,
    GC_POST_DECODE_AUDIO_SOURCE_STREAM_DESTROYED,
    GC_POST_DECODE_AUDIO_SINK_STREAM_CREATED,
    GC_POST_DECODE_AUDIO_SINK_STREAM_CLOSED,
    GC_POST_DECODE_AUDIO_SINK_STREAM_DESTROYED
} GC_STREAM_STATE_CHANGE_TYPE;

typedef struct GC_STREAM_STATE_CHANGE
{
   int64_t /* GC_STREAM_STATE_CHANGE_TYPE */ streamStateChangeType;
   union
   {
       PRE_ENCODE_AUDIO_STREAM_HANDLE preEncodeAudioStream;
       POST_DECODE_AUDIO_SOURCE_STREAM_HANDLE postDecodeAudioSourceStream;
       POST_DECODE_AUDIO_SINK_STREAM_HANDLE postDecodeAudioSinkStream;
   };
} GC_STREAM_STATE_CHANGE;

typedef const GC_STREAM_STATE_CHANGE * const * GC_STREAM_STATE_CHANGE_ARRAY;

typedef enum GC_AUDIO_MANIPULATION_MODE
{
    GC_AUDIO_MANIPULATION_MODE_NONE = 0x0,
    GC_AUDIO_MANIPULATION_MODE_PRE_ENCODE_ENABLED = 0x1,
    GC_AUDIO_MANIPULATION_MODE_POST_DECODE_ENABLED = 0x2,
} GC_AUDIO_MANIPULATION_MODE;

HRESULT
GC_API
ChatManagerSetMemFunctions(
    _In_opt_ GC_MEM_ALLOC_FUNC memAllocFunc,
    _In_opt_ GC_MEM_FREE_FUNC memFreeFunc
    );

HRESULT
GC_API
ChatManagerSetDebugMemoryMode(
    _In_ GC_DEBUG_MEMORY_MODE debugMemoryMode
    );

HRESULT
GC_API
ChatManagerGetDebugMemoryMode(
    _Out_ GC_DEBUG_MEMORY_MODE* debugMemoryMode
    );

HRESULT
GC_API
ChatManagerGetMemFunctions(
    _Out_ GC_MEM_ALLOC_FUNC* memAllocFunc,
    _Out_ GC_MEM_FREE_FUNC* memFreeFunc
    );

HRESULT
GC_API
ChatManagerSetThreadProcessor(
    _In_ GC_THREAD_ID threadId,
    _In_ uint32_t processorNumber
    );

HRESULT
GC_API
ChatManagerGetThreadProcessor(
    _In_ GC_THREAD_ID threadId,
    _Out_ uint32_t* processorNumber
    );

HRESULT
GC_API
ChatManagerSetThreadAffinityMask(
    _In_ GC_THREAD_ID threadId,
    _In_ uint64_t affinityMask
    );

HRESULT
GC_API
ChatManagerGetThreadAffinityMask(
    _In_ GC_THREAD_ID threadId,
    _Out_ uint64_t* affinityMask
    );

HRESULT
GC_API
ChatManagerSetLegacyUwpEraCompatModeEnabled(
    _In_ bool enabled
    );

HRESULT
GC_API
ChatManagerInitialize(
    _In_ uint32_t maxChatUserCount,
    _In_range_(0, 1) float defaultAudioRenderVolume,
    _In_ GC_COMMUNICATION_RELATIONSHIP defaultLocalToRemoteCommunicationRelationship,
    _In_ GC_SHARED_DEVICE_COMMUNICATION_RELATIONSHIP_RESOLUTION_MODE sharedDeviceResolutionMode,
    _In_ GC_SPEECH_TO_TEXT_CONVERSION_MODE speechToTextConversionMode,
    _In_ GC_AUDIO_MANIPULATION_MODE audioManipulationMode
    );

HRESULT
GC_API
ChatManagerCleanup(
    );

HRESULT
GC_API
ChatManagerSetAudioEncodingBitrate(
    _In_ GC_AUDIO_ENCODING_BITRATE audioEncodingBiterate
    );

HRESULT
GC_API
ChatManagerGetAudioEncodingBitrate(
    _Out_ GC_AUDIO_ENCODING_BITRATE * audioEncodingBiterate
    );

HRESULT
GC_API
ChatManagerStartProcessingDataFrames(
    _Out_ uint32_t* dataFrameCount,
    _Outptr_result_buffer_(*dataFrameCount) const GC_DATA_FRAME*const** dataFrames
    );

HRESULT
GC_API
ChatManagerFinishProcessingDataFrames(
    _In_ const GC_DATA_FRAME*const* dataFrames
    );

HRESULT
GC_API
ChatManagerStartProcessingStateChanges(
    _Out_ uint32_t* stateChangeCount,
    _Outptr_result_buffer_(*stateChangeCount) const GC_STATE_CHANGE*const** stateChanges
    );

HRESULT
GC_API
ChatManagerFinishProcessingStateChanges(
    _In_ const GC_STATE_CHANGE*const* stateChanges
    );

HRESULT
GC_API
ChatManagerAddLocalUser(
    _In_ PCWSTR xboxUserId,
    _Out_ CHAT_USER_HANDLE * chatUser
    );

HRESULT
GC_API
ChatManagerAddRemoteUser(
    _In_ PCWSTR xboxUserId,
    _In_ uint64_t remoteConsoleIdentifier,
    _Out_ CHAT_USER_HANDLE * chatUser
    );

HRESULT
GC_API
ChatManagerRemoveUser(
    _In_ CHAT_USER_HANDLE chatUser
    );

HRESULT
GC_API
ChatUserGetXboxUserId(
    _In_ CHAT_USER_HANDLE chatUser,
    _Outptr_ PCWSTR* xboxUserId
    );

HRESULT
GC_API
ChatUserGetIsLocal(
    _In_ CHAT_USER_HANDLE chatUser,
    _Out_ bool* isLocal
    );

HRESULT
GC_API
ChatUserSetCustomUserContext(
    _In_ CHAT_USER_HANDLE chatUser,
    _In_opt_ void* customUserContext
    );

HRESULT
GC_API
ChatUserGetCustomUserContext(
    _In_ CHAT_USER_HANDLE chatUser,
    _Outptr_result_maybenull_ void** customUserContext
    );

HRESULT
GC_API
ChatUserSetCommunicationRelationship(
    _In_ CHAT_USER_HANDLE chatUser,
    _In_ CHAT_USER_HANDLE targetChatUser,
    _In_ GC_COMMUNICATION_RELATIONSHIP communicationRelationship
    );

HRESULT
GC_API
ChatUserGetEffectiveCommunicationRelationship(
    _In_ CHAT_USER_HANDLE chatUser,
    _In_ CHAT_USER_HANDLE targetChatUser,
    _Out_ GC_COMMUNICATION_RELATIONSHIP * communicationRelationship,
    _Out_ GC_COMMUNICATION_RELATIONSHIP_ADJUSTER * communicationRelationshipAdjuster
    );

HRESULT
GC_API
ChatUserGetAudioRenderVolume(
    _In_ CHAT_USER_HANDLE chatUser,
    _In_ CHAT_USER_HANDLE targetUser,
    _Out_ float * volume
    );

HRESULT
GC_API
ChatUserSetAudioRenderVolume(
    _In_ CHAT_USER_HANDLE chatUser,
    _In_ CHAT_USER_HANDLE targetUser,
    _In_range_(0, 1) float volume
    );

HRESULT
GC_API
ChatUserGetMicrophoneMuted(
    _In_ CHAT_USER_HANDLE chatUser,
    _Out_ bool * muted
    );

HRESULT
GC_API
ChatUserSetMicrophoneMuted(
    _In_ CHAT_USER_HANDLE chatUser,
    _In_ bool muted
    );

HRESULT
GC_API
ChatUserGetRemoteUserMuted(
    _In_ CHAT_USER_HANDLE chatUser,
    _In_ CHAT_USER_HANDLE targetUser,
    _Out_ bool * muted
    );

HRESULT
GC_API
ChatUserSetRemoteUserMuted(
    _In_ CHAT_USER_HANDLE chatUser,
    _In_ CHAT_USER_HANDLE targetUser,
    _In_ bool muted
    );

HRESULT
GC_API
ChatUserSendChatText(
    _In_ CHAT_USER_HANDLE chatUser,
    _In_ PCWSTR message
    );

HRESULT
GC_API
ChatUserSynthesizeTextToSpeech(
    _In_ CHAT_USER_HANDLE chatUser,
    _In_ PCWSTR message
    );

HRESULT
GC_API
ChatUserGetSpeechToTextConversionPreferenceEnabled(
    _In_ CHAT_USER_HANDLE ximPlayer,
    _Out_ bool* speechToTextConversionPreferenceEnabled
    );

HRESULT
GC_API
ChatUserGetTextToSpeechConversionPreferenceEnabled(
    _In_ CHAT_USER_HANDLE ximPlayer,
    _Out_ bool* textToSpeechConversionPreferenceEnabled
    );

HRESULT
GC_API
ChatUserGetChatIndicator(
    _In_ CHAT_USER_HANDLE ximPlayer,
    _Out_ GC_CHAT_USER_CHAT_INDICATOR* chatPlayerChatIndicator
    );

HRESULT
GC_API
ChatManagerProcessIncomingPacket(
    _In_ uint64_t senderConsoleIdentifier,
    _In_ uint32_t bufferByteCount,
    _In_reads_(bufferByteCount) const void* buffer
    );

HRESULT
GC_API
ChatManagerGetAudioManipulationMode(
    _Out_ GC_AUDIO_MANIPULATION_MODE* audioManipulationMode
    );

HRESULT
GC_API
ChatManagerGetChatUsers(
    _Out_ uint32_t* chatUserCount,
    _Outptr_result_buffer_(*chatUserCount) const CHAT_USER_HANDLE** chatUsers
    );

HRESULT
GC_API
ChatManagerGetPreEncodeAudioStreams(
    _Out_ uint32_t* preEncodeAudioStreamCount,
    _Outptr_result_buffer_(*preEncodeAudioStreamCount) PRE_ENCODE_AUDIO_STREAM_ARRAY* preEncodeAudioStreams
    );

HRESULT
GC_API
ChatManagerGetPostDecodeAudioSourceStreams(
    _Out_ uint32_t* postDecodeAudioSourceStreamCount,
    _Outptr_result_buffer_(*postDecodeAudioSourceStreamCount) POST_DECODE_AUDIO_SOURCE_STREAM_ARRAY* postDecodeAudioSourceStreams
    );

HRESULT
GC_API
ChatManagerGetPostDecodeAudioSinkStreams(
    _Out_ uint32_t* postDecodeAudioSinkStreamCount,
    _Outptr_result_buffer_(*postDecodeAudioSinkStreamCount) POST_DECODE_AUDIO_SINK_STREAM_ARRAY* postDecodeAudioSinkStreams
    );

HRESULT
GC_API
ChatManagerStartProcessingStreamStateChanges(
    _Out_ uint32_t* streamStateChangeCount,
    _Outptr_result_buffer_(*streamStateChangeCount) GC_STREAM_STATE_CHANGE_ARRAY* streamStateChanges
    );

HRESULT
GC_API
ChatManagerFinishProcessingStreamStateChanges(
    _In_ GC_STREAM_STATE_CHANGE_ARRAY streamStateChanges
    );

HRESULT
GC_API
PreEncodeAudioStreamGetUsers(
    _In_ PRE_ENCODE_AUDIO_STREAM_HANDLE handle,
    _Out_ uint32_t* chatUserCount,
    _Outptr_result_buffer_(*chatUserCount) CHAT_USER_ARRAY* chatUsers
    );

HRESULT
GC_API
PreEncodeAudioStreamGetPreprocessedFormat(
    _In_ PRE_ENCODE_AUDIO_STREAM_HANDLE handle,
    _Out_ GC_AUDIO_FORMAT* format
    );

HRESULT
GC_API
PreEncodeAudioStreamSetProcessedFormat(
    _In_ PRE_ENCODE_AUDIO_STREAM_HANDLE handle,
    _In_ GC_AUDIO_FORMAT format
    );

HRESULT
GC_API
PreEncodeAudioStreamGetAvailableBufferCount(
    _In_ PRE_ENCODE_AUDIO_STREAM_HANDLE handle,
   _Out_ uint32_t* bufferCount
   );

HRESULT
GC_API
PreEncodeAudioStreamGetNextBuffer(
    _In_ PRE_ENCODE_AUDIO_STREAM_HANDLE handle,
    _Out_ uint32_t* byteCount,
    _Outptr_result_bytebuffer_maybenull_(*byteCount) void** buffer
    );

HRESULT
GC_API
PreEncodeAudioStreamReturnBuffer(
    _In_ PRE_ENCODE_AUDIO_STREAM_HANDLE handle,
    _In_ void* buffer
    );

HRESULT
GC_API
PreEncodeAudioStreamSubmitBuffer(
    _In_ PRE_ENCODE_AUDIO_STREAM_HANDLE handle,
    _In_ uint32_t byteCount,
    _In_reads_bytes_(byteCount) void* mixedBuffer
    );

HRESULT
GC_API
PreEncodeAudioStreamSetCustomStreamContext(
    _In_ PRE_ENCODE_AUDIO_STREAM_HANDLE handle,
    _In_ void* customStreamContext
    );

HRESULT
GC_API
PreEncodeAudioStreamGetCustomStreamContext(
    _In_ PRE_ENCODE_AUDIO_STREAM_HANDLE handle,
    _Outptr_ void** customStreamContext
    );

HRESULT
GC_API
PreEncodeAudioStreamIsOpen(
    _In_ PRE_ENCODE_AUDIO_STREAM_HANDLE handle,
    _Out_ bool* isClosed
    );

HRESULT
GC_API
PostDecodeAudioSourceStreamGetUsers(
    _In_ POST_DECODE_AUDIO_SOURCE_STREAM_HANDLE handle,
    _Out_ uint32_t* chatUserCount,
    _Outptr_result_buffer_(*chatUserCount) CHAT_USER_ARRAY* chatUsers
    );

HRESULT
GC_API
PostDecodeAudioSourceStreamGetPreprocessedFormat(
    _In_ POST_DECODE_AUDIO_SOURCE_STREAM_HANDLE handle,
    _Out_ GC_AUDIO_FORMAT* format
    );

HRESULT
GC_API
PostDecodeAudioSourceStreamGetAvailableBufferCount(
    _In_ POST_DECODE_AUDIO_SOURCE_STREAM_HANDLE handle,
   _Out_ uint32_t* bufferCount
   );

HRESULT
GC_API
PostDecodeAudioSourceStreamGetNextBuffer(
    _In_ POST_DECODE_AUDIO_SOURCE_STREAM_HANDLE handle,
    _Out_ uint32_t* byteCount,
    _Outptr_result_bytebuffer_maybenull_(*byteCount) void** buffer
    );

HRESULT
GC_API
PostDecodeAudioSourceStreamReturnBuffer(
    _In_ POST_DECODE_AUDIO_SOURCE_STREAM_HANDLE handle,
    _In_ void* buffer
    );

HRESULT
GC_API
PostDecodeAudioSourceStreamSetCustomStreamContext(
    _In_ POST_DECODE_AUDIO_SOURCE_STREAM_HANDLE handle,
    _In_ void* customStreamContext
    );

HRESULT
GC_API
PostDecodeAudioSourceStreamGetCustomStreamContext(
    _In_ POST_DECODE_AUDIO_SOURCE_STREAM_HANDLE handle,
    _Outptr_ void** customStreamContext
    );

HRESULT
GC_API
PostDecodeAudioSourceStreamIsOpen(
    _In_ POST_DECODE_AUDIO_SOURCE_STREAM_HANDLE handle,
    _Out_ bool* isClosed
    );

HRESULT
GC_API
PostDecodeAudioSinkStreamGetUsers(
    _In_ POST_DECODE_AUDIO_SINK_STREAM_HANDLE handle,
    _Out_ uint32_t* chatUserCount,
    _Outptr_result_buffer_(*chatUserCount) CHAT_USER_ARRAY* chatUsers
    );

HRESULT
GC_API
PostDecodeAudioSinkStreamSetProcessedFormat(
    _In_ POST_DECODE_AUDIO_SINK_STREAM_HANDLE handle,
    _In_ GC_AUDIO_FORMAT format
    );

HRESULT
GC_API
PostDecodeAudioSinkStreamSubmitMixedBuffer(
    _In_ POST_DECODE_AUDIO_SINK_STREAM_HANDLE handle,
    _In_ uint32_t byteCount,
    _In_reads_bytes_(byteCount) void* mixedBuffer
    );

HRESULT
GC_API
PostDecodeAudioSinkStreamCanReceiveAudioFromSourceStream(
    _In_ POST_DECODE_AUDIO_SINK_STREAM_HANDLE sinkHandle,
    _In_ POST_DECODE_AUDIO_SOURCE_STREAM_HANDLE sourceHandle,
    _Out_opt_ float* volume,
    _Out_ bool* canReceiveAudio
    );

HRESULT
GC_API
PostDecodeAudioSinkStreamGetDeviceId(
    _In_ POST_DECODE_AUDIO_SINK_STREAM_HANDLE handle,
    _Out_ PCWSTR* deviceId
    );

HRESULT
GC_API
PostDecodeAudioSinkStreamSetCustomStreamContext(
    _In_ POST_DECODE_AUDIO_SINK_STREAM_HANDLE handle,
    _In_ void* customStreamContext
    );

HRESULT
GC_API
PostDecodeAudioSinkStreamGetCustomStreamContext(
    _In_ POST_DECODE_AUDIO_SINK_STREAM_HANDLE handle,
    _Outptr_ void** customStreamContext
    );

HRESULT
GC_API
PostDecodeAudioSinkStreamIsOpen(
    _In_ POST_DECODE_AUDIO_SINK_STREAM_HANDLE handle,
    _Out_ bool* isClosed
    );

void
__declspec(noreturn)
GC_API
GameChatFailFastWithInform(
    _In_ HRESULT gameChatErrorCode,
    _In_ PCWSTR sourecFunction
);

#ifdef __cplusplus
}
#endif
