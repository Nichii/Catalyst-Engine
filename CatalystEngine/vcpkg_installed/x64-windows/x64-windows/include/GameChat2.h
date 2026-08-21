#pragma once

#ifndef __cplusplus
#error "This header requires C++"
#endif // end if not defined __cplusplus

namespace xbox { namespace services {

/// <summary>
/// Contains classes and enumerations for the Game Chat 2 library.
/// </summary>
namespace game_chat_2 {

/// <summary>
/// Maximum length in characters of a chat text message, not including the null terminator.
/// </summary>
constexpr uint16_t c_maxChatTextMessageLength = 1023;

/// <summary>
/// Maximum supported length in characters of an Xbox User ID, not including the null terminator.
/// </summary>
constexpr uint8_t c_maxXboxUserIdLength = 64;

/// <summary>
/// A processor number value that represents all processors available to the application currently running, rather than
/// a specific processor.
/// </summary>
constexpr uint32_t c_anyProcessorNumber = 0xFFFFFFFF;

/// <summary>
/// A processor affinity mask that represents all processors available to the application currently running, rather
/// than specific processors.
/// </summary>
constexpr uint64_t c_anyProcessor = 0xFFFFFFFFFFFFFFFF;

/// <summary>
/// Defines types of game_chat_state_change objects that can be reported by
/// chat_manager::start_processing_state_changes().
/// </summary>
enum class game_chat_state_change_type : int64_t
{
    /// <summary>
    /// A player-to-player text communication message has arrived.
    /// </summary>
    /// <remarks>
    /// The game_chat_state_change object should be cast to a game_chat_text_chat_received_state_change object for more
    /// information.
    /// </remarks>
    text_chat_received,

    /// <summary>
    /// A remote player's voice communication has been transcribed and should be displayed to one or more local users.
    /// </summary>
    /// <remarks>
    /// The game_chat_state_change object should be cast to a game_chat_transcribed_chat_received_state_change object
    /// for more information.
    /// </remarks>
    transcribed_chat_received,

    /// <summary>
    /// A local user has enabled or disabled speech-to-text or text-to-speech conversion.
    /// </summary>
    /// <remarks>
    /// The game_chat_state_change object should be cast to a game_chat_text_conversion_preference_changed_state_change
    /// object for more information.
    /// </remarks>
    text_conversion_preference_changed,

    /// <summary>
    /// A local user's communication relationship adjuster with respect to another user has changed.
    /// </summary>
    /// <remarks>
    /// The game_chat_state_change object should be cast to a game_chat_communication_relationship_adjusters_changed
    /// object for more information.
    /// </remarks>
    communication_relationship_adjuster_changed,
};

/// <summary>
/// Defines the data transport requirements for outgoing data.
/// </summary>
/// <seealso cref="game_chat_data_frame" />
enum class game_chat_data_transport_requirement
{
    /// <summary>
    /// The data must be transmitted with guaranteed delivery.
    /// </summary>
    /// <remarks>
    /// The data delivery must be guaranteed, regardless of environmental packet loss, unless the target recipient has
    /// been removed from the app's network before delivery completion. Sequentialness is not required.
    /// </remarks>
    guaranteed,

    /// <summary>
    /// The data should be transmitted with best effort delivery.
    /// </summary>
    /// <remarks>
    /// The data delivery should be sent best effort; environmental packet loss is acceptable. Sequentialness is not
    /// required.
    /// </remarks>
    best_effort,
};

/// <summary>
/// Defines the current chat status of a user in relationship to the local users.
/// </summary>
/// <remarks>
/// This is intended to assist UI iconic representation so that users can see why they are hearing/seeing what they're
/// hearing/seeing (or not). It changes frequently and is typically polled every graphics frame.
/// </remarks>
/// <seealso cref="chat_user::chat_indicator" />
enum class game_chat_user_chat_indicator
{
    /// <summary>
    /// The user is not currently talking.
    /// </summary>
    /// <remarks>
    /// Users that have recently been added to the local chat_manager instance are silent until Game Chat has finished
    /// performing asynchronous privacy and privilege checks.
    /// </remarks>
    silent,

    /// <summary>
    /// The user is currently talking.
    /// </summary>
    talking,

    /// <summary>
    /// The user's local microphone is muted.
    /// </summary>
    /// <seealso cref="chat_user::chat_user_local::set_microphone_muted" />
    /// <seealso cref="chat_user::chat_user_local::microphone_muted" />
    local_microphone_muted,

    /// <summary>
    /// The remote user has been muted by all local users.
    /// </summary>
    /// <seealso cref="chat_user::chat_user_local::set_remote_user_muted" />
    /// <seealso cref="chat_user::chat_user_local::remote_user_muted" />
    incoming_communications_muted,

    /// <summary>
    /// Chat with this user is limited because the user has an "Avoid Me" reputation designated by the Xbox Live
    /// service and Game Chat is applying a adjuster of type game_chat_communication_adjuster_type::reputation for
    /// one or more local users.
    /// </summary>
    reputation_restricted,

    /// <summary>
    /// Chat with this user is limited due to platform restrictions involving one or more local users.
    /// </summary>
    platform_restricted,

    /// <summary>
    /// The user is not able to chat because the app hasn't specified the microphone capability in its AppXManifest
    /// or a user has changed chat audio focus away from the app.
    /// </summary>
    no_chat_focus,

    /// <summary>
    /// The user does not have a microphone available or configured.
    /// </summary>
    /// <remarks>
    /// This state will only appear for local users. If chat communication with a remote user without a microphone
    /// would otherwise be restricted, such as due to muting or teams, the remote user will appear as having
    /// that restriction. Otherwise, a remote user without a microphone will appear as silent.
    /// </remarks>
    no_microphone,
};

/// <summary>
/// Defines the types of threads that Game Chat uses for internal purposes.
/// </summary>
/// <seealso cref="chat_manager::get_thread_processor" />
/// <seealso cref="chat_manager::set_thread_processor" />
enum class game_chat_thread_id
{
    /// <summary>
    /// Represents Game Chat internal audio threads.
    /// </summary>
    /// <remarks>
    /// Game Chat internal audio threads are high priority, frequently-running threads with real-time requirements.
    /// These interact directly with XAudio2 every 20-40 milliseconds.
    /// <para>
    /// When calling chat_manager::set_thread_processor() to configure the processor for this thread type, Game Chat 2's
    /// instance(s) of XAudio2 will be initialized with a processor affinity that corresponds to the configured processor
    /// number. If no processor number is specified for this thread type, Game Chat 2's instance(s) of XAudio2 will be
    /// initialized with a processor affinity of XAUDIO2_DEFAULT_PROCESSOR
    /// </para>
    /// </remarks>
    audio,

    /// <summary>
    /// Represents Game Chat internal networking threads.
    /// </summary>
    /// <remarks>
    /// Game Chat internal networking threads are driven by polling mechanisms. These wake every 50 to 100 milliseconds.
    /// </remarks>
    networking,
};

/// <summary>
/// Defines encoding bitrate Game Chat uses for generating outgoing audio packets.
/// </summary>
/// <remarks>
/// A lower bitrate results in less bandwidth usage but lower quality audio. The recommendation is to start at the
/// highest bitrate and move to lower bitrates when under bandwidth pressure.
/// </remarks>
/// <seealso cref="chat_manager::initialize" />
/// <seealso cref="chat_manager::audio_encoding_bitrate" />
/// <seealso cref="chat_manager::set_audio_encoding_bitrate" />
enum class game_chat_audio_encoding_bitrate
{
    /// <summary>
    /// Represents encoding with a 12 kilobits per second bitrate.
    /// </summary>
    kilobits_per_second_12,

    /// <summary>
    /// Represents encoding with a 16 kilobits per second bitrate.
    /// </summary>
    kilobits_per_second_16,

    /// <summary>
    /// Represents encoding with a 24 kilobits per second bitrate.
    /// </summary>
    kilobits_per_second_24,
};

/// <summary>
/// Defines if Game Chat should perform automatic speech-to-text conversion.
/// </summary>
/// <remarks>
/// Game Chat has the ability to automatically manage most work to support speech-to-text accessibility scenarios.
/// This enum allows games to disable the automatic work in favor of an independent speech-to-text solution.
/// </remarks>
enum class game_chat_speech_to_text_conversion_mode
{
    /// <summary>
    /// Represents the 'automatic' speech-to-text conversion mode.
    /// </summary>
    /// <remarks>
    /// If a local user has requested speech-to-text, Game Chat will automatically transcribe outgoing audio associated
    /// with the local user and incoming audio associated with users that are allowed to speak to the local user. Each
    /// transcription will be provided to the app as a game_chat_transcribed_chat_received_state_change. The app should
    /// display this message on screen for the user that has requested speech-to-text.
    /// </remarks>
    automatic,

    /// <summary>
    /// Represents the 'title managed' speech-to-text conversion mode.
    /// </summary>
    /// <remarks>
    /// Game Chat will not automatically transcribe audio when a user has requested speech-to-text. The title is
    /// responsible for transcribing audio when a user has the speech-to-text setting enabled (as determined
    /// by chat_user::chat_user_local::speech_to_text_conversion_preference_enabled()).
    /// </remarks>
    title_managed,
};

/// <summary>
/// Defines how Game Chat should resolve communication relationship conflicts for users that are on a shared device,
/// such as Kinect.
/// </summary>
/// <remarks>
/// This enum is used to indicate to Game Chat how communication relationship conflicts for users that are sharing
/// and audio device (e.g. Kinect) should be resolved.
/// </remarks>
enum class game_chat_shared_device_communication_relationship_resolution_mode
{
    /// <summary>
    /// Game Chat should choose the most permissive relationship when determining if audio from a shared device should
    /// be sent to remote users or render for local users.
    /// </summary>
    /// <remarks>
    /// This is equivalent to a bitwise 'or' operation on the relationships of the users on the shared device.
    /// </remarks>
    permissive,

    /// <summary>
    /// Game Chat should choose the most restrictive relationship when determining if audio from a shared device should
    /// be sent to remote users or render for local users.
    /// </summary>
    /// <remarks>
    /// This is equivalent to a bitwise 'and' operation on the relationships of the users on the shared device.
    /// </remarks>
    restrictive
};

/// <summary>
/// Flags used to define the communication relationship between two users.
/// </summary>
/// <seealso cref="chat_user::chat_user_local::set_communication_relationship" />
/// <seealso cref="chat_user::chat_user_local::get_effective_communication_relationship" />
enum class game_chat_communication_relationship_flags
{
    /// <summary>
    /// No communication between the local user and the target user is allowed.
    /// </summary>
    none = 0x0,

    /// <summary>
    /// Microphone communication from the local user to the target user is allowed.
    /// </summary>
    send_microphone_audio = 0x1,

    /// <summary>
    /// Text-to-speech communication from the local user to the target user is allowed.
    /// </summary>
    send_text_to_speech_audio = 0x2,

    /// <summary>
    /// Text communication from the local user to the target user is allowed.
    /// </summary>
    send_text = 0x4,

    /// <summary>
    /// Audio communication from the target user to the local user is allowed.
    /// </summary>
    receive_audio = 0x8,

    /// <summary>
    /// Text communication from the target user to the local user is allowed.
    /// </summary>
    receive_text = 0x10,
};

/// <summary>
/// A convenience operator definition for performing bitwise 'or' operations on
/// <em>game_chat_communication_relationship_flags</em> types.
/// </summary>
/// <seealso cref="game_chat_communication_relationship_flags" />
inline game_chat_communication_relationship_flags operator|
    (game_chat_communication_relationship_flags left, game_chat_communication_relationship_flags right)
{
    int32_t intLeft = static_cast<int32_t>(left);
    int32_t intRight = static_cast<int32_t>(right);
    return static_cast<game_chat_communication_relationship_flags>(intLeft | intRight);
}

/// <summary>
/// A convenience operator definition for performing bitwise 'and' operations on
/// <em>game_chat_communication_relationship_flags</em> types.
/// </summary>
/// <seealso cref="game_chat_communication_relationship_flags" />
inline game_chat_communication_relationship_flags operator&
    (game_chat_communication_relationship_flags left, game_chat_communication_relationship_flags right)
{
    int32_t intLeft = static_cast<int32_t>(left);
    int32_t intRight = static_cast<int32_t>(right);
    return static_cast<game_chat_communication_relationship_flags>(intLeft & intRight);
}

/// <summary>
/// A convenient definition for when a communication relationship allows sending all types of communication.
/// </summary>
/// <seealso cref="game_chat_communication_relationship_flags" />
constexpr game_chat_communication_relationship_flags c_communicationRelationshipSendAll =
    static_cast<game_chat_communication_relationship_flags>(
        static_cast<int32_t>(game_chat_communication_relationship_flags::send_microphone_audio) |
        static_cast<int32_t>(game_chat_communication_relationship_flags::send_text_to_speech_audio) |
        static_cast<int32_t>(game_chat_communication_relationship_flags::send_text));

/// <summary>
/// A convenient definition for when a communication relationship allows receiving all types of communication.
/// </summary>
/// <seealso cref="game_chat_communication_relationship_flags" />
constexpr game_chat_communication_relationship_flags c_communicationRelationshipReceiveAll =
    static_cast<game_chat_communication_relationship_flags>(
        static_cast<int32_t>(game_chat_communication_relationship_flags::receive_audio) |
        static_cast<int32_t>(game_chat_communication_relationship_flags::receive_text));

/// <summary>
/// A convenient definition for when a communication relationship allows sending and receiving all types of
/// communication.
/// </summary>
/// <seealso cref="game_chat_communication_relationship_flags" />
constexpr game_chat_communication_relationship_flags c_communicationRelationshipSendAndReceiveAll =
    static_cast<game_chat_communication_relationship_flags>(
        static_cast<int32_t>(c_communicationRelationshipSendAll) |
        static_cast<int32_t>(c_communicationRelationshipReceiveAll));

/// <summary>
/// A convenient definition for when a communication relationship allows sending and receiving text
/// communication.
/// </summary>
/// <seealso cref="game_chat_communication_relationship_flags" />
constexpr game_chat_communication_relationship_flags c_textRelationshipSendAndReceiveAll =
    static_cast<game_chat_communication_relationship_flags>(
        static_cast<int32_t>(game_chat_communication_relationship_flags::send_text) |
        static_cast<int32_t>(game_chat_communication_relationship_flags::receive_text));

/// <summary>
/// A convenient definition for when a communication relationship allows sending and receiving voice
/// communication.
/// </summary>
/// <seealso cref="game_chat_communication_relationship_flags" />
constexpr game_chat_communication_relationship_flags c_voiceRelationshipSendAndReceiveAll =
    static_cast<game_chat_communication_relationship_flags>(
        static_cast<int32_t>(game_chat_communication_relationship_flags::send_microphone_audio) |
        static_cast<int32_t>(game_chat_communication_relationship_flags::send_text_to_speech_audio) |
        static_cast<int32_t>(game_chat_communication_relationship_flags::receive_audio));

/// <summary>
/// Defines the extra adjustment steps applied by Game Chat to a communication relationship that has been set
/// by a previous call to chat_user::chat_user_local::set_communication_relationship().
/// </summary>
/// <remarks>
/// The adjusters are in order of precedence; i.e. if a particular relationship is effected by both privilege
/// restrictions (on the local user) and privacy restrictions (on the remote user), the adjuster will be reported as
/// 'privilege'.
/// </remarks>
/// <seealso cref="chat_user::chat_user_local::set_communication_relationship" />
/// <seealso cref="chat_user::chat_user_local::get_effective_communication_relationship" />
enum class game_chat_communication_relationship_adjuster
{
    /// <summary>
    /// No relationship has been set by the app for the pair of users.
    /// </summary>
    /// <remarks>
    /// Game Chat won't being initializing the adjusters for a pair of users until a relationship has been set that
    /// allows communication in at least on direction.
    /// </remarks>
    uninitialized,

    /// <summary>
    /// Game Chat is initializing information about one of the target users regarding privilege, privacy, or reputation.
    /// </summary>
    /// <remarks>
    /// The effective communication will always be 'none' when the adjuster is in this state.
    /// </remarks>
    initializing,

    /// <summary>
    /// Game Chat is applying no extra adjustment to this relationship.
    /// </summary>
    /// <remarks>
    /// The effective relationship will be what was set by the most recent call to
    /// chat_user::chat_user_local::set_communication_relationship().
    /// </remarks>
    none,

    /// <summary>
    /// Game Chat is restricting communication due to the local user's communication privilege that users might
    /// be able to resolve with UI.
    /// </summary>
    /// <remarks>
    /// The send_microphone_audio, send_text_to_speech_audio and send_text relationship flags are set to 0 when this
    /// adjuster is applied.
    ///
    /// The app can call XUserResolvePrivilegeWithUiAsync with XUserPrivilegeOptions::None and XUserPrivilege::Communications
    /// to attempt to resolve the issue. The user may be unable or unwilling to resolve the issue. If the user does resolve
    /// the issue, it will take effect next time the user is added to Game Chat.
    /// </remarks>
    privilege,

    /// <summary>
    /// Game Chat is restricting communication due to the local user's privacy settings in relation to the remote user.
    /// </summary>
    /// <remarks>
    /// The receive_audio and receive_text relationship flags are always set to 0 when this adjuster is applied. The
    /// send_microphone_audio, send_text_to_speech_audio and send_text relationship flags may be disallowed based on
    /// the nature of the privacy restriction.
    /// </remarks>
    privacy,

    /// <summary>
    /// Game Chat is restricting communication because the user has an "Avoid Me" reputation designated by the Xbox Live
    /// service.
    /// </summary>
    /// <remarks>
    /// The receive_audio and receive_text relationship flags are set to 0 when this adjuster is applied.
    /// </remarks>
    reputation,

    /// <summary>
    /// The user is on a shared device, such as Kinect, and the relationship has been modified due to the other user's
    /// relationship settings.
    /// </summary>
    /// <remarks>
    /// The conflict is resolved based on the settings determined in chat_manager::initialize().
    /// </remarks>
    /// <seealso cref="chat_manager::initialize" />
    conflict_with_other_user,

    /// <summary>
    /// Game Chat is restricting communication due to a failure to retrieve the user's privileges.
    /// </summary>
    /// <remarks>
    /// The send_microphone_audio, send_text_to_speech_audio and send_text relationship flags are set to 0 when this
    /// adjuster is applied.
    /// </remarks>
    privilege_check_failure,

    /// <summary>
    /// Game Chat is restricting communication due to a failure to retrieve the user's privileges that users might be
    /// able to resolve with UI.
    /// </summary>
    /// <remarks>
    /// The send_microphone_audio, send_text_to_speech_audio and send_text relationship flags are set to 0 when this
    /// adjuster is applied.
    ///
    /// <para>
    /// The app can call XUserResolveIssueWithUiAsync with nullptr for the url to attempt to resolve the issue.
    /// The user may be unable or unwilling to resolve the issue. If the user does resolve the issue, it will
    /// take effect next time the user is added to Game Chat.
    /// </para>
    /// </remarks>
    resolve_user_issue,

    /// <summary>
    /// Game Chat is restricting communication because Xbox Live is temporarily unavailable and user communication settings couldn't be retrieved.
    /// </summary>
    service_unavailable,
};

/// <summary>
/// Convenience type for a constant array of chat_user pointers.
/// </summary>
/// <seealso cref="chat_user" />
typedef class chat_user * const * chat_user_array;

/// <summary>
/// Represents an individual local or remote user that has been added to the local chat_manager instance.
/// </summary>
/// <seealso cref="chat_user::chat_user_local" />
/// <seealso cref="chat_manager" />
class chat_user
{
public:
    /// <summary>
    /// A subclass encapsulating the operations possible on local users only.
    /// </summary>
    /// <remarks>
    /// Calling the chat_user::local() method returns a pointer to an instance of the chat_user::chat_user_local class
    /// if the user is local, and null otherwise.
    /// </remarks>
    /// <seealso cref="chat_user" />
    /// <seealso cref="chat_user::local" />
    class chat_user_local
    {
    public:

        /// <summary>
        /// Sets the communication relationship between the local user and a target user.
        /// </summary>
        /// <remarks>
        /// Sets the communication relationship between the local user and a target user. The target user can be either
        /// remote or local. The default relationship between the local and a remote target user will be the
        /// default relationship specified in chat_manager::initialize(). The default relationship between the local
        /// user and a local target user will always be #none.
        /// </remarks>
        /// <param name="targetUser">The target user. The relationship is such that it can described as \"the local user
        /// is allowed to send to and/or receive from \p targetUser\". </param>
        /// <param name="communicationRelationship">The communication relationship between the local user and the
        /// target user.</param>
        /// <seealso cref="game_chat_communication_relationship_flags" />
        /// <seealso cref="chat_user::chat_user_local::get_effective_communication_relationship" />
        void set_communication_relationship(
            _In_ chat_user * targetUser,
            _In_ game_chat_communication_relationship_flags communicationRelationship
            ) noexcept;

        /// <summary>
        /// Gets the communication relationship that Game Chat is enforcing between the local user and a target user.
        /// </summary>
        /// <remarks>
        /// The direction of the relationship is such that it can be described as \"the local user can perform actions
        /// defined by \p communicationRelationship in relation to \p targetUser.\" E.g. if \p communicationRelationship is
        /// #receive_only that means \"this local user can receive communications from \p targetUser, but the local user
        /// cannot send communications to \p targetUser\".
        /// The communication relationship that Game Chat enforces for a pair of users can be different from what was
        /// set by the title because Game Chat performs extra adjustments based on privilege, privacy, and reputation.
        /// </remarks>
        /// <param name="targetUser">The target user.</param>
        /// <param name="effectiveCommunicationRelationship">The communication relationship that Game Chat is
        /// enforcing.</param>
        /// <param name="communicationRelationshipAdjuster">The adjuster being applied to the communication relationship
        /// set by the title. If \p effectiveCommunicationRelationship is different from what was set, this indicates
        /// why.</param>
        /// <seealso cref="game_chat_communication_relationship_flags" />
        /// <seealso cref="chat_user::chat_user_local::set_communication_relationship" />
        void get_effective_communication_relationship(
            _In_ chat_user * targetUser,
            _Out_ game_chat_communication_relationship_flags * effectiveCommunicationRelationship,
            _Out_ game_chat_communication_relationship_adjuster * communicationRelationshipAdjuster
            ) const noexcept;

        /// <summary>
        /// Provides the current volume setting for audio received from the target user that will be used for the local
        /// user.
        /// </summary>
        /// <remarks>
        /// The volume is a fractional percentage between 0.0 (quietest possible) and 1.0 (the standard chat volume).
        /// Values outside the [0, 1] range are clamped to the nearest extremum.
        /// <para>
        /// If the local user is on a shared audio device, such as Kinect, the effective volume will be the minimum of
        /// all volumes set the target user and local users on the shared device. E.g. if User A and User B are on a
        /// Kinect, and User A has set remote User C's volume to 0.5, while User B has set remote User C's volume to
        /// 0.6, Game Chat will render remote User C's audio to the Kinect with a volume of 0.5.
        /// </para>
        /// </remarks>
        /// <param name="targetUser">The target user whose volume should be provided.</param>
        /// <seealso cref="chat_user::chat_user_local::set_audio_render_volume" />
        /// <seealso cref="chat_user::chat_user_local::set_microphone_muted" />
        /// <seealso cref="chat_user::chat_user_local::set_remote_user_muted" />
        _Ret_range_(0, 1)
        float audio_render_volume(
            _In_ chat_user * targetUser
            ) const noexcept;

        /// <summary>
        /// Configures the volume setting for audio received from a target user that will be used for the local user.
        /// </summary>
        /// <remarks>
        /// The volume is a fractional percentage between 0.0 (quietest possible) and 1.0 (the standard chat volume).
        /// Values outside the [0, 1] range are clamped to the nearest extremum.
        /// <para>
        /// This is a local setting that takes effect immediately.
        /// </para>
        /// <para>
        /// If the local user is on a shared audio device, such as Kinect, the effective volume will be the minimum of
        /// all volumes set the target user and local users on the shared device. E.g. if User A and User B are on a
        /// Kinect, and User A has set remote User C's volume to 0.5, while User B has set remote User C's volume to
        /// 0.6, Game Chat will render remote User C's audio to the Kinect with a volume of 0.5.
        /// </para>
        /// </remarks>
        /// <param name ="targetUser">The user who's volume to configure in relation to the local user. The target user
        /// can be either local or remote. The default volume for remote target users is 1.0, while the default volume
        /// for local target users is 0.0.</param>
        /// <param name="volume">The new volume setting.</param>
        /// <seealso cref="chat_user::chat_user_local::audio_render_volume" />
        void set_audio_render_volume(
            _In_ chat_user * targetUser,
            _In_range_(0, 1) float volume
            ) noexcept;

        /// <summary>
        /// Returns whether the user's microphone has been previously configured to be in the muted state.
        /// </summary>
        /// <remarks>
        /// Muting determines whether audio data will be captured from the user's microphone and sent to the appropriate
        /// users according to the configured relationships. When the user's microphone is muted, no microphone audio
        /// data will be captured, regardless of the relationships that have been configured. Muting does not stop
        /// outgoing text messages or audio that has been generated by a call to
        /// chat_user::chat_user_local::synthesize_text_to_speech().
        /// <para>
        /// A user's microphone mute state can be modified using the chat_user::chat_user_local::set_microphone_muted() method.
        /// </para>
        /// </remarks>
        /// <seealso cref="chat_user::chat_user_local::set_microphone_muted" />
        /// <seealso cref="chat_user::chat_user_local::audio_render_volume" />
        /// <seealso cref="chat_user::chat_indicator" />
        bool microphone_muted(
            ) const noexcept;

        /// <summary>
        /// Configures whether the user's microphone is in the mute state.
        /// </summary>
        /// <remarks>
        /// Muting determines whether audio data will be captured from the user's microphone and sent to the appropriate
        /// users according to the configured relationships. When the user's microphone is muted, no microphone audio
        /// data will be captured, regardless of the relationships that have been configured. Muting does not stop
        /// outgoing text messages or audio that has been generated by a call to
        /// chat_user::chat_user_local::synthesize_text_to_speech().
        /// <para>
        /// The mute state applies to the user's device. As such, if the user is on a shared device, such as Kinect,
        /// this call will effect the microphone mute state of all users on the shared device.
        /// </para>
        /// </remarks>
        /// <param name="muted">Whether the user's microphone should be muted.</param>
        /// <seealso cref="chat_user::chat_user_local::set_microphone_muted" />
        /// <seealso cref="chat_user::chat_user_local::audio_render_volume" />
        /// <seealso cref="chat_user::chat_indicator" />
        void set_microphone_muted(
            _In_ bool muted
            ) noexcept;

        /// <summary>
        /// Returns whether the remote user has previously been muted in relation to the local user.
        /// </summary>
        /// <remarks>
        /// Muting determines whether incoming communications (both audio and text) from the target user will be
        /// be presented to the local user. When the target user is muted by the local user, the incoming
        /// communications will not be presented to the local user; the remote user's audio will not be rendered for the
        /// local user, and the local user will never be in the receiver list for a text message from the remote user.
        /// <para>
        /// A remote user's mute state can be modified using the chat_user::chat_user_local::set_remote_user_muted()
        /// method.
        /// </para>
        /// </remarks>
        /// <seealso cref="chat_user::set_remote_user_muted" />
        /// <seealso cref="chat_user::chat_user_local::audio_render_volume" />
        /// <seealso cref="chat_user::chat_indicator" />
        bool remote_user_muted(
            _In_ chat_user * targetUser
            ) noexcept;

        /// <summary>
        /// Configures whether the remote user is muted in relation to the local user.
        /// </summary>
        /// <remarks>
        /// Muting determines whether communications (both audio and text) will be presented to the local user. When
        /// muted, the remote user's audio will not be rendered for the local user and the local user will never been
        /// in the receiver list for a text message from the remote user.
        /// <para>
        /// If the local user is on a shared device, such as Kinect, the mute state for the target user will apply for
        /// all local users on the shared device.
        /// </para>
        /// </remarks>
        /// <seealso cref="chat_user::chat_user_local::remote_user_muted" />
        /// <seealso cref="chat_user::chat_user_local::audio_render_volume" />
        /// <seealso cref="chat_user::chat_indicator" />
        void set_remote_user_muted(
            _In_ chat_user * targetUser,
            _In_ bool muted
            ) noexcept;

        /// <summary>
        /// Generates a chat text string that will be sent to all users in the local chat_manager instance that are
        /// configured to receive communication from this user.
        /// </summary>
        /// <remarks>
        /// An outgoing data packet will be generated containing the text string with target users that are allowed to
        /// receive communications from this user based on privilege, privacy, and relationship configurations.
        /// <para>
        /// There is no guaranteed translation, localization, or offensive language filtering of the text content; the
        /// outgoing data packet will be generated with the text as is.
        /// </para>
        /// </remarks>
        /// <param name="message">The message string to transmit. This string must contain at least one and no more than
        /// <see cref="c_maxChatTextMessageLength" /> characters, not including the null terminator.</param>
        void send_chat_text(
            _In_ PCWSTR message
            ) noexcept;

        /// <summary>
        /// Synthesizes the text message as audio and generates audio packets as if the audio came from the user's
        /// microphone.
        /// </summary>
        /// <remarks>
        /// This should only be called if the user has the text-to-speech setting enabled, which is indicated by
        /// chat_user::speech_to_text_conversion_preference_enabled(). If the user does not have the text-to-speech
        /// setting enabled, this call will have no effect.
        /// <para>
        /// On Xbox One, the synthesis operation happens in the SRA and does not consume ERA resources. When the
        /// synthesis operation completes, the audio will be resampled in the ERA to match the format of the user's
        /// physical microphone, if one exists. CPU and memory usage for the resampling operation varies depending
        /// on the length of the synthesized audio buffer and the format of the user's physical microphone.
        /// </para>
        /// </remarks>
        /// <param name="message">The message for which audio should be synthesized.</param>
        void synthesize_text_to_speech(
            _In_ PCWSTR message
            ) noexcept;

        /// <summary>
        /// Returns true if this user has enabled speech-to-text conversion.
        /// </summary>
        /// <remarks>
        /// If this user has enabled speech-to-text conversion, this method returns true. Apps may listen for a
        /// game_chat_chat_text_conversion_preference_changed_state_change to know when a local player enables or
        /// disables this preference.
        /// <para>
        /// When this preference is enabled, Game Chat will automatically transcribe outgoing audio associated with the
        /// local user and incoming audio associated with users that are allowed to speak to the local user. Each
        /// transcription will be provided to the app as a game_chat_transcribed_chat_received_state_change. The app
        /// should display this message on screen for the user that has requested speech-to-text. Apps should render
        /// these messages similarly to other text messages, but should also show an icon or indicator that the message
        /// was transcribed from speech.
        /// </para>
        /// <para>
        /// Apps can use the value of this preference to customize the user interface to support incoming transcribed
        /// speech-to-text messages, for example by showing a caption pane.
        /// </para>
        /// <para>
        /// When speech-to-text is enabled by a local user in a Game Chat session, the Game Chat client on each remote
        /// device initiates a web socket connection with the speech services endpoint. Each remote Game Chat client
        /// uploads audio to the speech services endpoint through this websocket; the speech services endpoint
        /// occasionally gives back a transcription to the remote device. The remote device then sends the transcription
        /// message (i.e. a text message) to the local device, where the transcribed message is given by Game Chat to
        /// the app to render. The additional CPU + memory cost comes from managing this websocket; on Xbox One, the
        /// websocket management happens in the SRA and does not consume ERA resources. The majority of the websocket
        /// traffic is the upload of encoded audio data to the speech services endpoint. The audio data uploaded is used
        /// from Game Chat's internal encoding operations for data going to other users in session; as such, the
        /// bandwidth is proportional to the bitrate configured by a call to
        /// set_audio_encoding_bitrate().
        /// </para>
        /// </remarks>
        /// <seealso cref="chat_user::chat_user_local::text_to_speech_conversion_preference_enabled" />
        /// <seealso cref="chat_user::chat_user_local::send_chat_text" />
        /// <seealso cref="chat_manager::set_audio_encoding_bitrate" />
        /// <seealso cref="chat_manager::audio_encoding_bitrate" />
        bool speech_to_text_conversion_preference_enabled() const noexcept;

        /// <summary>
        /// Returns true if this user has enabled text-to-speech conversion.
        /// </summary>
        /// <remarks>
        /// If this user has enabled text-to-speech conversion, this method returns true. Apps may listen for a
        /// game_chat_chat_text_conversion_preference_changed_state_change to know when a local player enables or
        /// disables this preference.
        /// <para>
        /// If this preference is enabled, when chat_user::chat_user_local::synthesize_text_to_speech() is called for
        /// this user, remote users will hear synthesized audio representing the contents of the message.
        /// </para>
        /// <para>
        /// Apps can use the value of this preference to customize the user interface to support sending synthesized
        /// text-to-speech messages, for example by enabling a dedicated button to bring up the text input keyboard.
        /// </para>
        /// </remarks>
        /// <seealso cref="chat_user::chat_user_local::speech_to_text_conversion_preference_enabled" />
        /// <seealso cref="chat_user::chat_user_local::send_chat_text" />
        bool text_to_speech_conversion_preference_enabled() const noexcept;

    private:
        chat_user_local() = delete;
        chat_user_local(const chat_user_local&) = delete;
        chat_user_local(chat_user_local&&) = delete;
        chat_user_local& operator=(const chat_user_local&) = delete;
        chat_user_local& operator=(chat_user_local&&) = delete;
        ~chat_user_local() = delete;
    };

    /// <summary>
    /// Returns the unique Xbox User ID of this user.
    /// </summary>
    /// <remarks>
    /// The memory for the returned string pointer remains valid for the life of the chat_user object, which is until
    /// the user has been removed by a call to chat_manager::remove_user().
    /// </remarks>
    _Ret_z_
    PCWSTR xbox_user_id() const noexcept;

    /// <summary>
    /// Returns a pointer to the chat_user_local object associated to this user if the user is local; null otherwise.
    /// </summary>
    /// <remarks>
    /// The returned object pointer remains valid for the life of the chat_user object, which is until the user has been
    /// removed by a call to chat_manager::remove_user().
    /// </remarks>
    /// <seealso cref="chat_user::chat_user_local" />
    _Ret_maybenull_
    chat_user_local * local() noexcept;

    /// <summary>
    /// Returns a pointer to the chat_user_local object associated to this user if the user is local; null otherwise.
    /// </summary>
    /// <remarks>
    /// The returned object pointer remains valid for the life of the chat_user object, which is until the user has been
    /// removed by a call to chat_manager::remove_user().
    /// </remarks>
    /// <seealso cref="chat_user::chat_user_local" />
    _Ret_maybenull_
    const chat_user_local * local() const noexcept;

    /// <summary>
    /// Configures an optional, custom pointer-sized context value with this user object.
    /// </summary>
    /// <remarks>
    /// The custom user context is typically used as a "shortcut" that simplifies accessing local, app-specific memory
    /// associated with the user without requiring a mapping lookup. The value is retrieved using the
    /// chat_user::custom_user_context() method.
    /// <para>
    /// Any configured value is treated as opaque by the library, and is only valid on the local device; it is not
    /// transmitted over the network.
    /// </para>
    /// </remarks>
    /// <param name="customUserContext">An arbitrary, pointer-sized value to store with the user object.</param>
    /// <seealso cref="chat_user::custom_user_context" />
    void set_custom_user_context(
        _In_opt_ void * customUserContext
        ) noexcept;

    /// <summary>
    /// Retrieves the app's private, custom pointer-sized context value previously associated with this user object.
    /// </summary>
    /// <remarks>
    /// If no custom user context has been set yet, this returns a null pointer.
    /// </remarks>
    /// <returns>The pointer-sized value previously associated.</returns>
    /// <seealso cref="chat_user::set_custom_user_context" />
    _Ret_maybenull_
    void * custom_user_context() const noexcept;

    /// <summary>
    /// Indicates whether chat is possible and if audio is currently being produced for this user.
    /// </summary>
    /// <remarks>
    /// This indicator is intended to assist UI iconic representation so that users can determine why they're hearing/
    /// seeing what they are ("who's that talking?"), or why not.
    /// <para>
    /// This value changes frequently and is typically polled every graphics frame.
    /// </para>
    /// <para>
    /// If post-decode audio manipulation is enabled, chat indicators will be set based on which post-decode audio source
    /// streams are attempting and allowed to talk to which post-decode audio sink streams. The chat indicator will be
    /// set regardless of how the app mixes the post-decode source streams together.
    /// </para>
    /// </remarks>
    /// <seealso cref="game_chat_user_chat_indicator" />
    /// <seealso cref="chat_user::chat_user_local::set_microphone_muted" />
    /// <seealso cref="game_chat_audio_manipulation_mode_flags::post_decode_stream_manipulation" />
    game_chat_user_chat_indicator chat_indicator() const noexcept;

private:
    chat_user() = delete;
    chat_user(const chat_user&) = delete;
    chat_user(chat_user&&) = delete;
    chat_user& operator=(const chat_user&) = delete;
    chat_user& operator=(chat_user&&) = delete;
    ~chat_user() = delete;
};

/// <summary>
/// A generic, base structure representation of an event, change in state, or outgoing data.
/// </summary>
/// <remarks>
/// game_chat_state_change structures are reported by chat_manager::start_processing_state_changes() for the app to
/// handle and then promptly pass back via the game_chat::finish_processing_state_changes() method.
/// <para>
/// The <em>state_change_type</em> field indicates what kind of state change occurred, and this base structure should
/// then be cast to a more specific derived structure to retrieve additional event-specific information.
/// </para>
/// </remarks>
/// <seealso cref="game_chat_state_change_type" />
/// <seealso cref="chat_manager::start_processing_state_changes" />
/// <seealso cref="chat_manager::finish_processing_state_changes" />
struct game_chat_state_change
{
    /// <summary>
    /// The specific type of the state change represented.
    /// </summary>
    /// <remarks>
    /// Use this field to determine which corresponding derived structure is represented by this game_chat_state_change
    /// structure header.
    /// </remarks>
    game_chat_state_change_type state_change_type;
};

/// <summary>
/// Information specific to the game_chat_state_change_type::text_chat_received type of state change.
/// </summary>
/// <seealso cref="game_chat_state_change" />
/// <seealso cref="game_chat_state_change_type::text_chat_received" />
/// <seealso cref="chat_user" />
/// <seealso cref="chat_user_array" />
/// <seealso cref="chat_user::chat_user_local::send_chat_text" />
struct game_chat_text_chat_received_state_change : game_chat_state_change
{
    /// <summary>
    /// A pointer to the chat user who originated the text message.
    /// </summary>
    /// <remarks>
    /// Chat user object pointers remain valid until the user has been removed from the local chat_manager instance via
    /// the chat_manager::remove_user() method.
    /// </remarks>
    chat_user * sender;

    /// <summary>
    /// The number of local destination users to whom text message is addressed and have entries in the
    /// <em>receivers</em> field array.
    /// </summary>
    uint32_t receiver_count;

    /// <summary>
    /// An array of pointers to one or more local destination players to whom the text message is addressed.
    /// </summary>
    /// <remarks>
    /// The array has the number of entries specified by the <em>receiver_count</em> field.
    /// <para>
    /// The array memory remains valid until this object has been returned using
    /// chat_manager::finish_processing_state_changes().
    /// Individual user object pointers remain valid until the user has been removed from the local chat_manager
    /// instance via the chat_manager::remove_user() method.
    /// </para>
    /// </remarks>
    _Field_size_(receiver_count) chat_user_array receivers;

    /// <summary>
    /// A pointer to the text string that was sent.
    /// </summary>
    /// <remarks>
    /// This originated from a call to chat_user::chat_user_local::send_chat_text().
    /// <para>
    /// The string may be up to <see cref="c_maxChatTextMessageLength" /> characters long, not including the null
    /// terminator.
    /// </para>
    /// <para>
    /// The string remains valid until this object has been returned using
    /// chat_manager::finish_processing_state_changes(). The app should make a copy of the text to display in a manner
    /// and duration described in the Game Chat communication policy guide document.
    /// </para>
    /// </remarks>
    PCWSTR message;
};

/// <summary>
/// Information specific to the game_chat_state_change_type::transcribed_chat_received type of state change.
/// </summary>
/// <seealso cref="game_chat_state_change" />
/// <seealso cref="game_chat_state_change_type::transcribed_chat_received" />
/// <seealso cref="chat_user" />
/// <seealso cref="chat_user_array" />
struct game_chat_transcribed_chat_received_state_change : game_chat_state_change
{
    /// <summary>
    /// A pointer to the chat user who originated the transcribed chat.
    /// </summary>
    /// <remarks>
    /// Chat user object pointers remain valid until the user has been removed from the local chat_manager instance via
    /// the chat_manager::remove_user() method.
    /// </remarks>
    chat_user * speaker;

    /// <summary>
    /// The number of local destination users who have requested speech-to-text and have entries in the
    /// <em>receivers</em> field array.
    /// </summary>
    uint32_t receiver_count;

    /// <summary>
    /// An array of pointers to one or more local destination players who have requested speech-to-text.
    /// </summary>
    /// <remarks>
    /// The array has the number of entries specified by the <em>receiver_count</em> field.
    /// <para>
    /// The array memory remains valid until this object has been returned using
    /// chat_manager::finish_processing_state_changes().
    /// Individual user object pointers remain valid until the user has been removed from the local chat_manager
    /// instance via the chat_manager::remove_user() method.
    /// </para>
    /// </remarks>
    _Field_size_(receiver_count) chat_user_array receivers;

    /// <summary>
    /// A pointer to the transcribed message.
    /// </summary>
    /// <remarks>
    /// This message was generated by transcribing the audio of the user associated with the <em>speaker</em> object.
    /// <para>
    /// The string may be up to <see cref="c_maxChatTextMessageLength" /> characters long, not including the null
    /// terminator.
    /// </para>
    /// <para>
    /// The string remains valid until this object has been returned using
    /// chat_manager::finish_processing_state_changes(). The app should make a copy of the text to display in a manner
    /// and duration described in the Game Chat communication policy guide document.
    /// </para>
    /// </remarks>
    PCWSTR message;
};

/// <summary>
/// Information specific to the game_chat_state_change_type::text_conversion_preference_changed type of state change.
/// </summary>
/// <seealso cref="game_chat_state_change" />
/// <seealso cref="game_chat_state_change_type::text_conversion_preference_changed" />
/// <seealso cref="chat_user" />
/// <seealso cref="chat_user::chat_user_local::speech_to_text_conversion_preference_enabled" />
/// <seealso cref="chat_user::chat_user_local::text_to_speech_conversion_preference_enabled" />
struct game_chat_text_conversion_preference_changed_state_change : game_chat_state_change
{
    /// <summary>
    /// A pointer to the local player who has enabled or disabled speech-to-text and/or text-to-speech conversion.
    /// </summary>
    /// <remarks>
    /// Apps should query chat_user::chat_user_local::speech_to_text_conversion_preference_enabled() and
    /// chat_user::chat_user_local::text_to_speech_conversion_preference_enabled() to get the new preference values.
    /// </remarks>
    chat_user * chatUser;
};

/// <summary>
/// Information specific to the game_chat_state_change_type::communication_relationship_adjuster_changed type of state
/// change.
/// </summary>
/// <remarks>
/// This state indicates the additional adjustments that Game Chat is applying to a communication relationship has
/// changed. This can occur under the following conditions:
/// 1. A communication relationship between a particular pair of users has been set for the first time; the adjuster is
///    expected to move into the #initializing state.
/// 2. The adjuster has finished initializing.
/// 3. The adjuster has changed due to action by the user. E.g. a privacy relationship has changed.
/// <para>
/// To determine the effective communication relationship and the adjuster that is applied, use
/// chat_user::chat_user_local::get_effective_communication_relationship().
///
///  game_chat_communication_relationship_flags effectiveRelationship;
///  communication_relationship_adjuster adjuster;
///  local_user->get_effective_communication_relationship(target_user, &effectiveRelationship, &adjuster);
///
/// </para>
/// </remarks>
/// <seealso cref="game_chat_state_change" />
/// <seealso cref="game_chat_state_change_type::communication_relationship_adjuster_changed" />
/// <seealso cref="chat_user::chat_user_local::set_communication_relationship" />
/// <seealso cref="chat_user::chat_user_local::get_effective_communication_relationship" />
struct game_chat_communication_relationship_adjuster_changed_state_change : game_chat_state_change
{
    /// <summary>
    /// The local user that is the base of the relationship.
    /// </summary>
    chat_user * local_user;

    /// <summary>
    /// The target user in the relationship. This user could be either local or remote.
    /// </summary>
    chat_user * target_user;
};

/// <summary>
/// Convenience type for a constant array of constant game_chat_state_change pointers.
/// </summary>
/// <seealso cref="game_chat_state_change" />
/// <seealso cref="chat_manager::start_processing_state_changes" />
/// <seealso cref="chat_manager::finish_processing_state_changes" />
typedef const game_chat_state_change * const * game_chat_state_change_array;

/// <summary>
/// Information about outgoing Game Chat data that must be delivered to remote Game Chat endpoints.
/// </summary>
/// <seealso cref="chat_manager::start_processing_data_frames" />
/// <seealso cref="chat_manager::finish_processing_data_frames" />
/// <seealso cref="chat_manager::add_remote_user" />
struct game_chat_data_frame
{
    /// <summary>
    /// The number of remote endpoints that this data must be transmitted to and have entries in the
    /// <em>target_endpoint_identifiers</em> field array.
    /// </summary>
    uint32_t target_endpoint_identifier_count;

    /// <summary>
    /// An array of one or more remote endpoint identifiers that this data be transmitted to.
    /// </summary>
    /// <remarks>
    /// The array has the number of entries specified by the <em>target_endpoint_identifier_count</em> field.
    /// <para>
    /// The array memory remains valid until this object has been returned using
    /// chat_manager::finish_processing_data_frames().
    /// </para>
    /// <para>
    /// The possible endpoint identifier values are those that have been previously specified by calls to
    /// chat_manager::add_remote_user().
    /// </para>
    /// </remarks>
    _Field_size_(target_endpoint_identifier_count) uint64_t * target_endpoint_identifiers;

    /// <summary>
    /// The transport requirements for the data.
    /// </summary>
    game_chat_data_transport_requirement transport_requirement;

    /// <summary>
    /// The size of the data buffer that must be transmitted to the remote endpoints specified by
    /// <em>target_endpoint_identifiers</em>.
    /// </summary>
    uint32_t packet_byte_count;

    /// <summary>
    /// A pointer to the data buffer that must be transmitted to the remote endpoints specified by
    /// <em>target_endpoint_identifiers</em>.
    /// </summary>
    _Field_size_(packet_byte_count) uint8_t * packet_buffer;
};

/// <summary>
/// A macro abstracting the calling convention for Game Chat library callbacks.
/// </summary>
#define game_chat_callback __stdcall

/// <summary>
/// A callback invoked every time a new memory buffer must be dynamically allocated by the Game Chat library.
/// </summary>
/// <remarks>
/// This callback is optionally installed using the chat_manager::set_memory_callbacks() method.
/// <para>
/// The callback must allocate and return a pointer to a contiguous block of memory of the specified size that will
/// remain valid until the app's corresponding <see cref="game_chat_free_memory_callback" /> is invoked to release it.
/// If this is not possible, the callback must return a null pointer to fail the allocation. Memory allocation failures
/// are sometimes considered benign but will usually cause current Game Chat library operation(s) to fail.
/// </para>
/// <para>
/// Every non-null pointer returned by this method will be subsequently passed to the corresponding
/// <see cref="game_chat_free_memory_callback" /> once the memory is no longer needed.
/// </para>
/// </remarks>
/// <param name="size">The size of the allocation to be made. This value will never be zero.</param>
/// <param name="memoryTypeId">An opaque identifier representing the Game Chat internal category of memory being
/// allocated.</param>
/// <returns>A pointer to an allocated block of memory of the specified size, or a null pointer if allocation
/// failed.</returns>
/// <seealso cref="game_chat_free_memory_callback" />
/// <seealso cref="chat_manager::set_memory_callbacks" />
/// <seealso cref="chat_manager::get_memory_callbacks" />
typedef
_Ret_maybenull_
_Post_writable_byte_size_(size) void *
(game_chat_callback * game_chat_allocate_memory_callback)(
    _In_ size_t size,
    _In_ uint32_t memoryTypeId
    );

/// <summary>
/// A callback invoked every time a previously allocated memory buffer is no longer needed by the Game Chat library and
/// can be freed.
/// </summary>
/// <remarks>
/// This callback is optionally installed using the chat_manager::set_memory_callbacks() method.
/// <para>
/// The callback is invoked whenever the Game Chat library has finished using a memory buffer previously returned by the
/// app's corresponding <see cref="game_chat_allocate_memory_callback" />, such that the application can free the
/// memory buffer.
/// </para>
/// </remarks>
/// <param name="pointer">The pointer to the memory buffer previously allocated. This value will never be a null
/// pointer.</param>
/// <param name="memoryTypeId">An opaque identifier representing the Game chat internal category of memory being
/// freed.</param>
/// <seealso cref="game_chat_allocate_memory_callback" />
/// <seealso cref="chat_manager::set_memory_callbacks" />
/// <seealso cref="chat_manager::get_memory_callbacks" />
typedef
void
(game_chat_callback * game_chat_free_memory_callback)(
    _In_ _Post_invalid_ void * pointer,
    _In_ uint32_t memoryTypeId
    );

/// <summary>
/// Convenience type for a constant array of constant game_chat_data_frame pointers.
/// </summary>
/// <seealso cref="game_chat_data_frame" />
/// <seealso cref="chat_manager::start_processing_data_frames" />
/// <seealso cref="chat_manager::finish_processing_data_frames" />
typedef const game_chat_data_frame * const * game_chat_data_frame_array;

/// <summary>
/// The data type used to represent a single Game Chat audio sample.
/// </summary>
enum class game_chat_sample_type
{
    /// <summary>
    /// Integer PCM format.
    /// </summary>
    integer = 0,

    /// <summary>
    /// IEEE floating-point PCM format.
    /// </summary>
    ieee_float
};

/// <summary>
/// The format information needed to interpret Game Chat audio data.
/// </summary>
/// <seealso cref="pre_encode_audio_stream::get_pre_processed_format" />
/// <seealso cref="pre_encode_audio_stream::set_processed_format" />
/// <seealso cref="post_decode_audio_source_stream::get_pre_processed_format" />
/// <seealso cref="post_decode_audio_sink_stream::set_processed_format" />
struct game_chat_audio_format
{
    /// <summary>
    /// Specifies the sample frequency at which each channel should be played or recorded.
    /// </summary>
    uint32_t sample_rate;

    /// <summary>
    /// Overrides the assignment of channels in a multichannel audio stream to speaker positions. For the bit-to-speaker
    /// mapping see the msdn page for WAVEFORMATEXTENSIBLE.
    /// https://msdn.microsoft.com/en-us/library/windows/hardware/ff538802(v=vs.85).aspx.
    /// To use the default mapping, set this to 0.
    /// </summary>
    uint32_t channel_mask;

    /// <summary>
    /// Specifies the number of channels of audio data.
    /// </summary>
    uint16_t channel_count;

    /// <summary>
    /// Specifies the number of bits per sample. If this value is not byte-divisible, it will be assumed that the
    /// containing sample type is padded with bits to make it byte-divisble.
    /// </summary>
    uint16_t bits_per_sample;

    /// <summary>
    /// Specifies whether we are using PCM data represented by integers
    /// (<see cref="game_chat_sample_format_type::integer" />) or floating-point numbers
    /// (<see cref="game_chat_sample_format_type::ieee_float" />).
    /// </summary>
    game_chat_sample_type sample_type;

    /// <summary>
    /// A flag representing whether the multichannel audio stream is interleaved for multi-channel formats.
    /// </summary>
    /// <remarks>
    /// Audio retrieved from Game Chat will always be marked as interleaved. This bool only has meaning if
    /// <see cref="game_chat_audio_format::channel_count" /> is greater than 1.
    /// </remarks>
    bool is_interleaved;
};

/// <summary>
/// Convenience type for a constant array of <see cref="pre_encode_audio_stream" /> pointers.
/// </summary>
/// <seealso cref="pre_encode_audio_stream" />
typedef class pre_encode_audio_stream * const * pre_encode_audio_stream_array;

/// <summary>
/// Represents the local audio manipulation pipeline. Audio generated by a set of users associated
/// with a capture device can be retrieved from a pre-encode audio stream, processed, and submitted back to the stream for
/// encoding and transmission as the audio of the set of users associated with the capture device.
/// </summary>
/// <remarks>
/// Any audio delivered through this stream will have already been preprocessed with Voice Activity Detection (VAD) and
/// Automatic Gain Control (AGC). Only buffers with voice activity will be available from this source.
/// </remarks>
/// <seealso cref="chat_manager::get_pre_encode_audio_streams" />
class pre_encode_audio_stream
{
public:
    /// <summary>
    /// The users that generate the audio retrieved from this source. The array pointer returned is valid until this
    /// stream is destroyed. If this stream was destroyed as the result of calling
    /// <see cref="chat_manager::remove_user"/>, the chat_user pointer for that removed user will also become invalid.
    /// </summary>
    /// <seealso cref="game_chat_stream_state_change_type::pre_encode_audio_stream_destroyed" />
    void get_users(
        _Out_ uint32_t* chatUserCount,
        _Outptr_result_buffer_(*chatUserCount) chat_user_array* chatUsers
        ) const noexcept;

    /// <summary>
    /// The format of the buffers that will be returned by <see cref="pre_encode_audio_stream::get_next_buffer" />.
    /// The pre-processed audio format will always be mono. Expect to handle samples represented as 32-bit floats,
    /// 16-bit integers, and 32-bit integers.
    /// </summary>
    game_chat_audio_format get_pre_processed_format(
        ) const noexcept;

    /// <summary>
    /// Specify the format of the processed audio that is submitted to Game Chat for encoding and transmission to
    /// remote endpoints.
    /// <see cref="pre_encode_audio_stream::submit_buffer" />.
    /// </summary>
    /// <remarks>
    /// This method must be called once before <see cref="pre_encode_audio_stream::submit_buffer" />
    /// is called, and cannot be called after. This method also may not be called after this stream closes.
    /// </remarks>
    /// <param name="format">
    /// The format of the audio submitted for Game Chat to encode and transmit. Only mono formats are valid. Only
    /// 32-bit float PCM, 32-bit integer PCM, and 16-bit integer PCM formats are currently supported for
    /// pre_encode_audio_streams. Valid samples rates are dependent on the platform. Xbox One ERA supports 8kHz, 12kHz, 16kHz,
    /// and 24kHz. UWP for Xbox One and PC supports 8kHz, 12kHz, 16kHz, 24kHz, 32kHz, 44.1kHz, and 48kHz.
    /// </param>
    /// <seealso cref="pre_encode_audio_stream::submit_buffer" />
    void set_processed_format(
        _In_ game_chat_audio_format format
        ) noexcept;

    /// <summary>
    /// The total number of buffers available to retrieve from this stream.
    /// </summary>
    /// <remarks>
    /// This can be useful if the caller prefers to send audio through their pipeline in batches of buffers. Because
    /// only 10 buffers will be held by the stream at any given time, it is recommended that callers not attempt to collect
    /// batches larger than 4 buffers. Callers should give their audio processing pipeline ample time to process the
    /// buffers and return them to <see cref="pre_encode_audio_stream::return_buffer" />, to prevent overloading their audio
    /// stream.
    /// </remarks>
    uint32_t get_available_buffer_count(
        ) const noexcept;

    /// <summary>
    /// Gets the next buffer available in the stream.
    /// </summary>
    /// <remarks>
    /// If there are any new audio buffers for this stream, they will be available for processing every 40ms. Buffers
    /// returned by this method must be released to <see cref="pre_encode_audio_stream::return_buffer" /> when they are done
    /// being used. A maximum of 10 buffers will be held on each pre-encode audio stream. Once this limit is reached, new buffers
    /// will be dropped, and old buffers must be released with <see cref="pre_encode_audio_stream::return_buffer" /> before
    /// new buffers will become available. Pre-encode audio streams can be filtered by a minimum amount of available data
    /// using <see cref="pre_encode_audio_stream::get_available_buffer_count" />.
    /// Buffers delivered through this stream have already been preprocessed with Voice Activity Detection (VAD) and
    /// Automatic Gain Control (AGC).
    /// To interpret the buffer, the caller should use <see cref="pre_encode_audio_stream::get_pre_processed_format" /> to
    /// examine the number of channels in the buffer, how the channels are arranged in the buffer, and the sample size
    /// of the buffer.
    /// </remarks>
    /// <param name="byteCount">
    /// A pointer to the number of bytes in the buffer. The number of samples in this buffer can be derived using the
    /// bits per sample value in this stream's pre-processed format. If no buffer is available, *byteCount will equal zero.
    /// </param>
    /// <param name="buffer">
    /// A pointer to the next buffer. If no buffer is available, *buffer will equal nullptr.
    /// </param>
    /// <seealso cref="pre_encode_audio_stream::get_pre_processed_format" />
    /// <seealso cref="pre_encode_audio_stream::return_buffer" />
    void get_next_buffer(
        _Out_ uint32_t* byteCount,
        _Outptr_result_bytebuffer_maybenull_(*byteCount) void** buffer
        ) noexcept;

    /// <summary>
    /// Tells Game Chat it can reclaim memory associated with this buffer.
    /// </summary>
    /// <remarks>
    /// This is *not* for submitting processed buffers. Use <see cref="pre_encode_audio_stream::submit_buffer" />.
    /// Buffers do not need to be returned in the order they were retrieved from
    /// <see cref="pre_encode_audio_stream::get_next_buffer" />. A maximum of 10 unreturned buffers can
    /// exist for any stream. When this limit is reached, the audio stream will drop any new buffers until more buffers
    /// are returned to this method.
    /// </remarks>
    /// <seealso cref="pre_encode_audio_stream::get_next_buffer" />
    void return_buffer(
        _In_ void* buffer
        ) noexcept;

    /// <summary>
    /// Submits audio to be associated with this stream's users and encoded for transmission to remote endpoints.
    /// </summary>
    /// <remarks>
    /// Because this stream is associated with a set of users, those users' privacy and privilege restrictions will be
    /// applied to this audio. As well, submitting audio for the wrong user will cause erroneous chat indicator
    /// behavior. Every 40ms, the next 40ms of audio from this stream will be encoded and transmitted. To prevent audio
    /// hiccups, buffers for audio that should be heard continuously should be submitted to this stream at a constant
    /// rate. After this stream has been closed, calls to submit buffers will silently fail. The app can check whether a
    /// stream is open or closed by calling <see cref="pre_encode_audio_stream::is_open" />.
    /// </remarks>
    /// <param name="byteCount">The number of bytes in the buffer.</param>
    /// <param name="buffer">
    /// The buffer of processed audio data. This buffer must comply with the format specified in
    /// <see cref="pre_encode_audio_stream::set_processed_format" />.
    /// </param>
    /// <seealso cref="pre_encode_audio_stream::set_processed_format" />
    /// <seealso cref="pre_encode_audio_stream::is_open" />
    void submit_buffer(
        _In_ uint32_t byteCount,
        _In_reads_bytes_(byteCount) void* buffer
        ) noexcept;

    /// <summary>
    /// Configures an optional, custom pointer-sized context value with this stream object.
    /// </summary>
    /// <remarks>
    /// The custom stream context is typically used as a "shortcut" that simplifies accessing local, app-specific memory
    /// associated with the stream without requiring a mapping lookup. The value is retrieved using the
    /// <see cref="pre_encode_audio_stream::custom_stream_context" />.
    /// <para>
    /// Any configured value is treated as opaque by the library, and is only valid on the local device; it is not
    /// transmitted over the network.
    /// </para>
    /// </remarks>
    /// <param name="customStreamContext">An arbitrary, pointer-sized value to store with the stream object.</param>
    /// <seealso cref="pre_encode_audio_stream::custom_stream_context" />
    void set_custom_stream_context(
        _In_ void* customStreamContext
        ) noexcept;

    /// <summary>
    /// Retrieves the app's private, custom pointer-sized context value previously associated with this stream object.
    /// </summary>
    /// <remarks>
    /// If no custom stream context has been set yet, this returns a null pointer.
    /// </remarks>
    /// <returns>The pointer-sized value previously associated.</returns>
    /// <seealso cref="pre_encode_audio_stream::set_custom_stream_context" />
    void* custom_stream_context(
        ) const noexcept;

    /// <summary>
    /// Returns true before this stream's corresponding <see cref="pre_encode_audio_stream_closed" /> stream state
    /// change is returned to Game Chat. After this stream state change has been returned, this method will return false.
    /// </summary>
    bool is_open(
        ) const noexcept;
};

/// <summary>
/// Convenience type for a constant array of <see cref="post_decode_audio_source_stream" /> pointers.
/// </summary>
/// <seealso cref="post_decode_audio_source_stream" />
typedef class post_decode_audio_source_stream * const * post_decode_audio_source_stream_array;

/// <summary>
/// Represents the inbound-data side of the remote audio manipulation pipeline. Audio generated by a set of users associated
/// with a capture device can be retrieved from a post-decode audio source stream, processed, and submitted to a
/// <see cref="post_decode_audio_sink_stream" /> for remote audio manipulation.
/// </summary>
/// <remarks>
/// Any audio delivered through this stream will have already been preprocessed with Voice Activity Detection (VAD) and
/// Automatic Gain Control (AGC). Only buffers with voice activity will be available from this source.
/// </remarks>
/// <seealso cref="post_decode_audio_sink_stream" />
/// <seealso cref="chat_manager::get_post_decode_audio_source_streams" />
class post_decode_audio_source_stream
{
public:
    /// <summary>
    /// The users that generate the audio retrieved from this source. The array pointer returned is valid until this
    /// stream is destroyed. If this stream was destroyed as the result of calling
    /// <see cref="chat_manager::remove_user"/>, the chat_user pointer for that removed user will also become invalid.
    /// </summary>
    void get_users(
        _Out_ uint32_t* chatUserCount,
        _Outptr_result_buffer_(*chatUserCount) chat_user_array* chatUsers
        ) const noexcept;

    /// <summary>
    /// The format of the buffers that will be returned by <see cref="post_decode_audio_source_stream::get_next_buffer" />.
    /// The pre-processed audio format will always be mono. The sample type will always be 16-bit integers. The sample
    /// rate will always be 24 kHz.
    /// </summary>
    game_chat_audio_format get_pre_processed_format(
        ) const noexcept;

    /// <summary>
    /// The total number of buffers available to retrieve from this stream.
    /// </summary>
    /// <remarks>
    /// This can be useful if the caller prefers to send audio through their pipeline in batches of buffers. Because
    /// only 10 buffers will be held by the stream at any given time, it is recommended that callers not attempt to collect
    /// batches larger than 4 buffers. Callers should give their audio processing pipeline ample time to process the
    /// buffers and return them to <see cref="post_decode_audio_source_stream::return_buffer" />, to prevent overflowing their audio
    /// stream.
    /// </remarks>
    uint32_t get_available_buffer_count(
        ) const noexcept;

    /// <summary>
    /// Gets the next buffer available in the stream.
    /// </summary>
    /// <remarks>
    /// If there are any new audio buffers for this stream, they will be available for processing every 40ms. Buffers
    /// returned by this method must be released to <see cref="post_decode_audio_source_stream::return_buffer" /> when they are done
    /// being used. A maximum of 10 buffers will be held on each audio stream. Once this limit is reached, new buffers
    /// will be dropped, and old buffers must be released with <see cref="post_decode_audio_source_stream::return_buffer" /> before
    /// new buffers will become available. Post-decode audio source streams can be filtered by a minimum amount of available data
    /// using <see cref="post_decode_audio_source_stream::get_available_buffer_count" />.
    /// Buffers delivered through this stream have already been preprocessed with Voice Activity Detection (VAD) and
    /// Automatic Gain Control (AGC).
    /// To interpret the buffer, the caller should use <see cref="post_decode_audio_source_stream::get_pre_processed_format" />
    /// to examine the number of channels in the buffer, how the channels are arranged in the buffer, and the sample size
    /// of the buffer.
    /// </remarks>
    /// <param name="byteCount">
    /// A pointer to the number of bytes in the buffer. The number of samples in this buffer can be derived using the
    /// bits per sample value in this stream's pre-processed format. If no buffer is available, *byteCount will equal zero.
    /// </param>
    /// <param name="buffer">
    /// A pointer to the next buffer. If no buffer is available, *buffer will equal nullptr.
    /// </param>
    /// <seealso cref="post_decode_audio_source_stream::get_pre_processed_format" />
    /// <seealso cref="post_decode_audio_source_stream::return_buffer" />
    void get_next_buffer(
        _Out_ uint32_t* byteCount,
        _Outptr_result_bytebuffer_maybenull_(*byteCount) void** buffer
        ) noexcept;

    /// <summary>
    /// Tells Game Chat it can reclaim memory associated with this buffer.
    /// </summary>
    /// <remarks>
    /// This is *not* for submitting processed buffers. Use <see cref="post_decode_audio_sink_stream::submit_mixed_buffer" />.
    /// Buffers do not need to be returned in the order they were retrieved from
    /// <see cref="post_decode_audio_source_stream::get_next_buffer" />. A maximum of 10 unreturned buffers can exist for any
    /// stream. When this limit is reached, the post-decode audio source stream will drop any new buffers until more buffers
    /// are returned to this method.
    /// </remarks>
    /// <seealso cref="post_decode_audio_source_stream::get_next_buffer" />
    void return_buffer(
        _In_ void* buffer
        ) noexcept;

    /// <summary>
    /// Configures an optional, custom pointer-sized context value with this stream object.
    /// </summary>
    /// <remarks>
    /// The custom stream context is typically used as a "shortcut" that simplifies accessing local, app-specific memory
    /// associated with the stream without requiring a mapping lookup. The value is retrieved using the
    /// <see cref="post_decode_audio_source_stream::custom_stream_context" />.
    /// <para>
    /// Any configured value is treated as opaque by the library, and is only valid on the local device; it is not
    /// transmitted over the network.
    /// </para>
    /// </remarks>
    /// <param name="customStreamContext">An arbitrary, pointer-sized value to store with the stream object.</param>
    /// <seealso cref="post_decode_audio_source_stream::custom_stream_context" />
    void set_custom_stream_context(
        _In_ void* customStreamContext
        ) noexcept;

    /// <summary>
    /// Retrieves the app's private, custom pointer-sized context value previously associated with this stream object.
    /// </summary>
    /// <remarks>
    /// If no custom stream context has been set yet, this returns a null pointer.
    /// </remarks>
    /// <returns>The pointer-sized value previously associated.</returns>
    /// <seealso cref="post_decode_audio_source_stream::set_custom_stream_context" />
    void* custom_stream_context(
        ) const noexcept;

    /// <summary>
    /// Returns true before this stream's corresponding <see cref="post_decode_audio_source_stream_closed" /> stream state
    /// change is returned to Game Chat. After this stream state change has been returned, this method will return false.
    /// </summary>
    bool is_open(
        ) const noexcept;
};

/// <summary>
/// Convenience type for a constant array of post_decode_audio_sink_stream pointers.
/// </summary>
/// <seealso cref="post_decode_audio_sink_stream" />
typedef class post_decode_audio_sink_stream * const * post_decode_audio_sink_stream_array;

/// <summary>
/// Represents the outbound-data side of the remote audio manipulation pipeline. Audio submitted to these streams will be
/// rendered to the stream's users.
/// </summary>
/// <remarks>
/// Only audio from a <see cref="post_decode_audio_source_stream" /> with a set of users who have the appropriate privileges to
/// talk with this sink's users should submit audio to this sink. See:
/// <see cref="post_decode_audio_sink_stream::submit_mixed_buffer" />.
/// </remarks>
/// <seealso cref="post_decode_audio_source_stream" />
/// <seealso cref="chat_manager::get_post_decode_audio_sink_streams" />
class post_decode_audio_sink_stream
{
public:
    /// <summary>
    /// The local users associated with this stream's render device. The array pointer returned is valid until this
    /// stream is destroyed. If this stream was destroyed as the result of calling
    /// <see cref="chat_manager::remove_user"/>, the chat_user pointer for that removed user will also become invalid.
    /// </summary>
    void get_users(
        _Out_ uint32_t* chatUserCount,
        _Outptr_result_buffer_(*chatUserCount) chat_user_array* chatUsers
        ) const noexcept;

    /// <summary>
    /// Specify the format of the processed audio that is submitted to Game Chat for rendering through
    /// <see cref="post_decode_audio_sink_stream::submit_mixed_buffer" />.
    /// </summary>
    /// <remarks>
    /// This method must be called once before <see cref="post_decode_audio_sink_stream::submit_mixed_buffer" /> is called and
    /// cannot be called after. This method also may not be called after this stream closes.
    /// </remarks>
    /// <param name="format">
    /// The format of the audio submitted for Game Chat to render. Interleaved formats are preferred. Non-interleaved
    /// formats will be interleaved for rendering which will incur a performance penalty.
    /// </param>
    /// <seealso cref="post_decode_audio_sink_stream::submit_mixed_buffer" />
    void set_processed_format(
        _In_ game_chat_audio_format format
        ) noexcept;

    /// <summary>
    /// Submits audio to the render device represented by this sink.
    /// </summary>
    /// <remarks>
    /// Every 40ms the next mixed buffer from this stream will be sent to render. To prevent audio hiccups, buffers for
    /// audio that should be heard continuously should be submitted to this stream at a constant rate. After this stream
    /// has been closed, calls to submit buffers will silently fail. The app can check whether a stream is open or closed
    /// by calling <see cref="post_decode_audio_sink_stream::is_open" />.
    /// </remarks>
    /// <param name="byteCount">The number of bytes in the buffer.</param>
    /// <param name="mixedBuffer">
    /// The buffer of processed and mixed audio data. Every audio buffer that is expected to play in unison for this
    /// sink's set of users must be mixed as one buffer and submitted in one call to this method. Any buffers submitted
    /// in sequential calls, will be played back sequentially. Callers are responsible for enforcing privacy and
    /// privilege between the source and sink users when mixing. Privacy and privilege relationships can be determined
    /// with <see cref="post_decode_audio_sink_stream::can_receive_audio_from_source_stream" />. This buffer must comply with the format
    /// specified in <see cref="post_decode_audio_sink_stream::set_processed_format" />.
    /// </param>
    /// <seealso cref="post_decode_audio_sink_stream::set_processed_format" />
    /// <seealso cref="post_decode_audio_sink_stream::is_open" />
    void submit_mixed_buffer(
        _In_ uint32_t byteCount,
        _In_reads_bytes_(byteCount) void* mixedBuffer
        ) noexcept;

    /// <summary>
    /// Determine whether Game Chat will allow audio communication from the users associated with a post-decode source
    /// stream to the users associated with this post-decode sink stream based on communication relationships and
    /// platform requirements such as system-level mutes. Also optionally provides a suggested volume that the source
    /// stream's audio should be mixed to.
    /// </summary>
    /// <remarks>
    /// This is a convenience method that is equivalent to calling
    /// <see cref="chat_user::get_effective_communication_relationship" /> between each source stream user and each sink
    /// stream user and coalescing the result.
    ///
    /// In the event of a shared device, this method will return permissive or restrictive results based on the shared
    /// device resolution mode specified in <see cref="chat_manager::initialize" /> and the app-specified communication
    /// relationships. Platform restrictions (e.g. mute or block) will be restrictive regardless of the device
    /// resolution mode.
    ///
    /// Privacy/privilege for a stream will not consider users that have been removed by calling
    /// <see cref="chat_manager::remove_user" />. If all of the users on the source or sink stream have been removed,
    /// then this method will return false and set the volume, if provided, to zero.
    /// </remarks>
    /// <param name="sourceStream">
    /// The post-decode source audio stream whose users we are evaluating for their communication relationships.
    /// </param>
    /// <param name="volume">
    /// The volume that Game Chat would mix the source stream's audio with if post-decode buffer manipulation was not
    /// enabled. This is a suggested volume, but apps may use their own heuristic for resolving volume. The volume
    /// returned will be the minimum render volume between each of the source users and each of the sink users,
    /// regardless of the shared device resolution mode, similar to <see cref="chat_user::audio_render_volume" />.
    /// </param>
    /// <seealso cref="chat_user::audio_render_volume" />.
    /// <seealso cref="game_chat_shared_device_communication_relationship_resolution_mode" />
    /// <seealso cref="chat_manager::initialize" />
    bool can_receive_audio_from_source_stream(
        _In_ post_decode_audio_source_stream* sourceStream,
        _Out_opt_ float* volume
        ) const noexcept;

    /// <summary>
    /// The ID of this sink stream's render device.
    /// </summary>
    /// <remarks>
    /// This is useful if the caller wants to render the audio data directly to a render device but would like
    /// to use Game Chat for selecting the render device. If the caller chooses to render their own audio, there is no
    /// need to call <see cref="post_decode_audio_sink_stream::submit_mixed_buffer" />.
    /// </remarks>
    /// <returns>A pointer to a null-terminated wide string representing the device ID.</returns>
    PCWSTR get_device_id(
        ) const noexcept;

    /// <summary>
    /// Configures an optional, custom pointer-sized context value with this stream object.
    /// </summary>
    /// <remarks>
    /// The custom stream context is typically used as a "shortcut" that simplifies accessing local, app-specific memory
    /// associated with the stream without requiring a mapping lookup. The value is retrieved using the
    /// <see cref="post_decode_audio_sink_stream::custom_stream_context" />.
    /// <para>
    /// Any configured value is treated as opaque by the library, and is only valid on the local device; it is not
    /// transmitted over the network.
    /// </para>
    /// </remarks>
    /// <param name="customStreamContext">An arbitrary, pointer-sized value to store with the stream object.</param>
    /// <seealso cref="post_decode_audio_sink_stream::custom_stream_context" />
    void set_custom_stream_context(
        _In_ void* customStreamContext
        ) noexcept;

    /// <summary>
    /// Retrieves the app's private, custom pointer-sized context value previously associated with this stream object.
    /// </summary>
    /// <remarks>
    /// If no custom stream context has been set yet, this returns a null pointer.
    /// </remarks>
    /// <returns>The pointer-sized value previously associated.</returns>
    /// <seealso cref="post_decode_audio_sink_stream::set_custom_stream_context" />
    void* custom_stream_context(
        ) const noexcept;

    /// <summary>
    /// Returns true before this stream's corresponding <see cref="post_decode_audio_sink_stream_closed" /> stream state
    /// change is returned to Game Chat. After this stream state change has been returned, this method will return false.
    /// </summary>
    bool is_open(
        ) const noexcept;
};

/// <summary>
/// Defines types of game_chat_stream_state_change objects that can be reported by
/// chat_manager::start_processing_stream_state_changes().
/// </summary>
enum class game_chat_stream_state_change_type : int64_t
{
    /// <summary>
    /// A pre-encode audio stream has been created by Game Chat for a capture device.
    /// </summary>
    /// <remarks>
    /// Use this state change as an opportunity to take care of any initialization that needs to happen before the
    /// stream starts returning buffers via <see cref="pre_encode_audio_stream::get_next_buffer" /> or starts accepting
    /// processed audio via <see cref="pre_encode_audio_stream::submit_buffer" />.
    /// Once this state change is returned to <see cref="chat_manager::finish_processing_stream_state_changes" />, the
    /// stream will become available for querying from <see cref="chat_manager::get_pre_encode_audio_streams" />.
    /// As well, the pre-encode audio stream will begin queueing audio accessible from
    /// <see cref="pre_encode_audio_stream::get_next_buffer" /> and will start accepting processed audio via
    /// <see cref="pre_encode_audio_stream::submit_buffer" />.  The stream is valid until an associated
    /// <see cref="game_chat_stream_state_change_type::pre_encode_audio_stream_destroyed" /> stream state change is returned
    /// back to Game Chat.
    /// This state change can only occur if pre-encode audio stream processing has been enabled.
    /// </remarks>
    /// <seealso cref="chat_manager::initialize" />
    /// <seealso cref="pre_encode_audio_stream" />
    pre_encode_audio_stream_created,

    /// <summary>
    /// A pre-encode audio stream for a capture device will not return any more buffers via
    /// <see cref="pre_encode_audio_stream::get_next_buffer" />, and will not accept more buffers via
    /// <see cref="pre_encode_audio_stream::submit_buffer" />.
    /// </summary>
    /// <remarks>
    /// This may be due to the capture device becoming unavailable or the users associated with the stream changing if
    /// the stream represented a shared capture device. The stream is still valid, but any buffers submitted to the
    /// stream will be silently dropped and calls to <see cref="pre_encode_audio_stream::set_processed_format" /> will
    /// fatally fail.
    /// Once this state change is returned to <see cref="chat_manager::finish_processing_stream_state_changes" />, the
    /// stream is no longer available to use for audio manipulation and should be cleaned up so that it
    /// can be destroyed and reclaimed by Game Chat.
    /// These streams are valid until an associated
    /// <see cref="game_chat_stream_state_change_type::pre_encode_audio_stream_destroyed" /> stream state change is returned
    /// back to Game Chat.
    /// This state change can only occur if pre-encode audio stream processing has been enabled.
    /// </remarks>
    /// <seealso cref="chat_manager::initialize" />
    /// <seealso cref="pre_encode_audio_stream" />
    pre_encode_audio_stream_closed,

    /// <summary>
    /// A capture device's corresponding pre-encode audio stream will be destroyed by Game Chat 2.
    /// </summary>
    /// <remarks>
    /// This state change will only occur when all buffers retrieved from
    /// <see cref="pre_encode_audio_stream::get_next_buffer" /> have been returned to
    /// <see cref="pre_encode_audio_stream::return_buffer" />, and no buffers submitted to
    /// <see cref="pre_encode_audio_stream::submit_buffer" /> are in use by the stream. After returning
    /// this state change to <see cref="chat_manager::finish_processing_stream_state_changes" />, its accompanying stream
    /// is invalid. Any custom stream context assigned to this stream should be cleaned up before this state change is
    /// returned.
    /// This state change can only occur if pre-encode audio stream processing has been enabled.
    /// </remarks>
    /// <seealso cref="chat_manager::initialize" />
    /// <seealso cref="pre_encode_audio_stream" />
    pre_encode_audio_stream_destroyed,

    /// <summary>
    /// A post-decode audio source stream has been created by Game Chat.
    /// </summary>
    /// <remarks>
    /// Use this state change as an opportunity to take care of any initialization that needs to happen before this
    /// stream starts queueing audio.
    /// Once this state change is returned to <see cref="chat_manager::finish_processing_stream_state_changes" />, the
    /// source stream will become available for querying from
    /// <see cref="chat_manager::get_post_decode_audio_source_streams" />, and will begin queueing audio accessible from
    /// <see cref="post_decode_audio_source_stream::get_next_buffer" />.
    /// The stream is valid until an associated
    /// <see cref="game_chat_stream_state_change_type::post_decode_audio_source_stream_destroyed" /> stream state change is
    /// returned back to Game Chat.
    /// This state change can only occur if remote audio stream processing has been enabled.
    /// </remarks>
    /// <seealso cref="chat_manager::initialize" />
    /// <seealso cref="post_decode_audio_source_stream" />
    post_decode_audio_source_stream_created,

    /// <summary>
    /// A post-decode audio source stream for a set of remote users will not return any more buffers through
    /// <see cref="post_decode_audio_source_stream::get_next_buffer" />.
    /// </summary>
    /// <remarks>
    /// This may be due to the remote capture device becoming unavailable or the users associated with the stream
    /// changing if the stream represented a shared remote capture device. The stream is still valid memory and should
    /// be used for returning buffers to Game Chat through <see cref="post_decode_audio_source_stream::return_buffer" />.
    /// Once this state change is returned to <see cref="chat_manager::finish_processing_stream_state_changes" />, the
    /// stream is no longer available to use for audio manipulation and should be cleaned up so that it
    /// can be destroyed and reclaimed by Game Chat.
    /// The stream is valid until an associated
    /// <see cref="game_chat_stream_state_change_type::post_decode_audio_source_stream_destroyed" /> stream state change is
    /// returned back to Game Chat.
    /// This state change can only occur if remote audio stream processing has been enabled.
    /// </remarks>
    /// <seealso cref="chat_manager::initialize" />
    /// <seealso cref="post_decode_audio_source_stream" />
    post_decode_audio_source_stream_closed,

    /// <summary>
    /// A post-decode audio source stream will be destroyed by Game Chat.
    /// </summary>
    /// <remarks>
    /// This state change will only occur when all buffers retrieved from
    /// <see cref="post_decode_audio_source_stream::get_next_buffer" /> have been returned to
    /// <see cref="post_decode_audio_source_stream::return_buffer" />. After returning this state change to
    /// <see cref="chat_manager::finish_processing_stream_state_changes" />, its accompanying stream is invalid. Any
    /// custom context assigned to this stream should be cleaned up before this state change is returned.
    /// This state change can only occur if remote audio stream processing has been enabled.
    /// </remarks>
    /// <seealso cref="chat_manager::initialize" />
    /// <seealso cref="post_decode_audio_source_stream" />
    post_decode_audio_source_stream_destroyed,

    /// <summary>
    /// A post-decode audio sink stream for a local render device has been created by Game Chat.
    /// </summary>
    /// <remarks>
    /// Use this state change as an opportunity to take care of any initialization that needs to happen before this
    /// stream starts to accept audio data via <see cref="post_decode_audio_sink_stream::submit_mixed_buffer" />.
    /// Once this state change is returned to <see cref="chat_manager::finish_processing_stream_state_changes" />, the
    /// sink stream will become available for querying from
    /// <see cref="chat_manager::get_post_decode_audio_sink_streams" /> and will start accepting processed audio via
    /// <see cref="post_decode_audio_sink_stream::submit_mixed_buffer" />.
    /// The stream is valid until an associated
    /// <see cref="game_chat_stream_state_change_type::post_decode_audio_sink_stream_destroyed" /> stream state change is
    /// returned back to Game Chat.
    /// This state change can only occur if remote audio stream processing has been enabled.
    /// </remarks>
    /// <seealso cref="chat_manager::initialize" />
    /// <seealso cref="post_decode_audio_sink_stream" />
    post_decode_audio_sink_stream_created,

    /// <summary>
    /// A post-decode audio sink stream for a local render device will not accept any more buffers through
    /// <see cref="post_decode_audio_sink_stream::submit_mixed_buffer" />.
    /// </summary>
    /// <remarks>
    /// This may be due to the render device becoming unavailable or the users associated with the render device
    /// changing if the stream represented a shared render device. The stream is still valid, but any buffers submitted
    /// to the stream will be silently dropped and calls to
    /// <see cref="post_decode_audio_sink_stream::set_processed_format" /> will fatally fail.
    /// Once this state change is returned to <see cref="chat_manager::finish_processing_stream_state_changes" />, the
    /// stream is no longer available to use for audio manipulation and should be cleaned up so that it
    /// can be destroyed and reclaimed by Game Chat.
    /// The stream is valid until an associated
    /// <see cref="game_chat_stream_state_change_type::post_decode_audio_sink_stream_destroyed" /> stream state change is
    /// returned back to Game Chat.
    /// This state change can only occur if remote audio stream processing has been enabled.
    /// </remarks>
    /// <seealso cref="chat_manager::initialize" />
    /// <seealso cref="post_decode_audio_sink_stream" />
    post_decode_audio_sink_stream_closed,

    /// <summary>
    /// A post-decode audio sink stream will be destroyed by Game Chat.
    /// </summary>
    /// <remarks>
    /// This state change will only occur when no buffers submitted to
    /// <see cref="post_decode_audio_sink_stream::submit_mixed_buffer" /> are in use by the stream. After returning this state
    /// change to <see cref="chat_manager::finish_processing_stream_state_changes" />, its accompanying stream is
    /// invalid. Any custom context assigned to this stream should be cleaned up before this state change is
    /// returned.
    /// This state change can only occur if remote audio stream processing has been enabled.
    /// </remarks>
    /// <seealso cref="chat_manager::initialize" />
    /// <seealso cref="post_decode_audio_sink_stream" />
    post_decode_audio_sink_stream_destroyed,
};

/// <summary>
/// Convenience type for a constant array of constant game_chat_stream_state_change pointers.
/// </summary>
/// <seealso cref="game_chat_stream_state_change" />
/// <seealso cref="chat_manager::start_processing_stream_state_changes" />
/// <seealso cref="chat_manager::finish_processing_stream_state_changes" />
typedef const struct game_chat_stream_state_change * const * game_chat_stream_state_change_array;

/// <summary>
/// A generic, base structure representation of an event for audio manipulation streams.
/// </summary>
/// <remarks>
/// game_chat_stream_state_change structures are reported by
/// <see cref="chat_manager::start_processing_stream_state_changes" /> for the app to handle and then promptly pass back
/// via the <see cref="chat_manager::finish_processing_state_changes" /> method.  These stream state changes will only
/// occur if local or remote audio processing is enabled in <see cref="chat_manager::initialize" />.
/// <para>
/// The <em>state_change_type</em> field indicates what kind of state change occurred, and this base structure should
/// then be cast to a more specific derived structure to retrieve additional event-specific information.
/// </para>
/// </remarks>
/// <seealso cref="chat_manager::initialize" />.
/// <seealso cref="game_chat_stream_state_change_type" />
/// <seealso cref="chat_manager::start_processing_stream_state_changes" />
/// <seealso cref="chat_manager::finish_processing_stream_state_changes" />
struct game_chat_stream_state_change
{
    /// <summary>
    /// The specific type of the stream state change represented.
    /// </summary>
    /// <remarks>
    /// Use this field to determine which type of stream field to read from the stream union.
    /// </remarks>
    game_chat_stream_state_change_type state_change_type;

    /// <summary>
    /// A union of all the different stream types this state change can carry.
    /// </summary>
    /// <remarks>
    /// If the state_change_type is a pre_encode_audio_stream state change, use the preEncodeAudioStream field.
    /// If the state_change_type is a post_decode_audio_source_stream state change, use the postDecodeAudioSourceStream field.
    /// If the state_change_type is a post_decode_audio_sink_stream state change, use the postDecodeAudioSinkStream field.
    /// </remarks>
    union
    {
        pre_encode_audio_stream * pre_encode_audio_stream;
        post_decode_audio_source_stream * post_decode_audio_source_stream;
        post_decode_audio_sink_stream * post_decode_audio_sink_stream;
    };
};

/// <summary>
/// Flags to indicate the requested audio manipulation feature status.
/// </summary>
/// <seealso cref="chat_manager::initialize" />
/// <seealso cref="chat_manager::audio_manipulation_mode" />
enum class game_chat_audio_manipulation_mode_flags
{
    /// <summary>
    /// All audio manipulation is disabled.
    /// </summary>
    none = 0x0,

    /// <summary>
    /// Manipulation of locally generated audio is enabled.
    /// </summary>
    pre_encode_stream_manipulation = 0x1,

    /// <summary>
    /// Manipulation of remotely generated audio is enabled.
    /// </summary>
    post_decode_stream_manipulation = 0x2,
};

/// <summary>
/// A convenience operator definition for performing bitwise 'or' operations on
/// <em>game_chat_audio_manipulation_mode_flags</em> types.
/// </summary>
/// <seealso cref="game_chat_audio_manipulation_mode_flags" />
inline game_chat_audio_manipulation_mode_flags operator|
    (game_chat_audio_manipulation_mode_flags left, game_chat_audio_manipulation_mode_flags right)
{
    int32_t intLeft = static_cast<int32_t>(left);
    int32_t intRight = static_cast<int32_t>(right);
    return static_cast<game_chat_audio_manipulation_mode_flags>(intLeft | intRight);
}

/// <summary>
/// A convenience operator definition for performing bitwise 'and' operations on
/// <em>game_chat_audio_manipulation_mode_flags</em> types.
/// </summary>
/// <seealso cref="game_chat_audio_manipulation_mode_flags" />
inline game_chat_audio_manipulation_mode_flags operator&
    (game_chat_audio_manipulation_mode_flags left, game_chat_audio_manipulation_mode_flags right)
{
    int32_t intLeft = static_cast<int32_t>(left);
    int32_t intRight = static_cast<int32_t>(right);
    return static_cast<game_chat_audio_manipulation_mode_flags>(intLeft & intRight);
}

/// <summary>
/// The primary management class for interacting with Game Chat.
/// </summary>
/// <remarks>
/// Only a single instance of the class is permitted.
/// </remarks>
/// <seealso cref="chat_user" />
class chat_manager
{
public:
    /// <summary>
    /// Retrieves a reference to the Game Chat singleton instance.
    /// </summary>
    static chat_manager& singleton_instance();

    /// <summary>
    /// Optionally configures the current memory allocation and freeing callbacks the Game Chat library should use.
    /// </summary>
    /// <remarks>
    /// This method allows the application to install custom memory allocation routines in order to service all requests
    /// by the Game Chat library for new memory buffers instead of using its default allocation routines.
    /// <para>
    /// The <paramref name="allocateMemoryCallback" /> and <paramref name="freeMemoryCallback" /> parameters can be null
    /// pointers to restore the default routines. Both callback pointers must be null or both must be non-null. Mixing
    /// custom and default routines is not permitted.
    /// </para>
    /// <para>
    /// This method must be called prior to the chat_manager::initialize() method. The callbacks cannot change while any
    /// allocations are outstanding. It also must be only called by one thread at a time as it isn't multithreading
    /// safe.
    /// </para>
    /// <para>
    /// The configured callbacks are persisted until changed, including across calls to chat_manager::cleanup().
    /// </para>
    /// </remarks>
    /// <param name="allocateMemoryCallback">A pointer to the custom allocation callback to use, or a null pointer to
    /// restore the default.</param>
    /// <param name="freeMemoryCallback">A pointer to the custom freeing callback to use, or a null pointer to restore
    /// the default.</param>
    /// <seealso cref="game_chat_allocate_memory_callback" />
    /// <seealso cref="game_chat_free_memory_callback" />
    /// <seealso cref="chat_manager::get_memory_callbacks" />
    static void set_memory_callbacks(
        _In_opt_ game_chat_allocate_memory_callback allocateMemoryCallback,
        _In_opt_ game_chat_free_memory_callback freeMemoryCallback
        ) noexcept;

    /// <summary>
    /// Retrieves the current memory allocation and freeing callbacks the Game Chat library is using.
    /// </summary>
    /// <remarks>
    /// This retrieves the memory allocation routines servicing requests by the Game Chat library for new memory.
    /// <para>
    /// This method does not require the chat_manager::initialize() method to have been called first.
    /// </para>
    /// </remarks>
    /// <param name="allocateMemoryCallback">A place to store a pointer to the memory allocation callback currently
    /// used.</param>
    /// <param name="freeMemoryCallback">A place to store a pointer to the memory freeing callback currently
    /// used.</param>
    /// <seealso cref="game_chat_allocate_memory_callback" />
    /// <seealso cref="game_chat_free_memory_callback" />
    /// <seealso cref="chat_manager::set_memory_callbacks" />
    static void get_memory_callbacks(
        _Out_ game_chat_allocate_memory_callback * allocateMemoryCallback,
        _Out_ game_chat_free_memory_callback * freeMemoryCallback
        ) noexcept;

    /// <summary>
    /// Optionally configures the processor on which internal Game Chat threads will run.
    /// </summary>
    /// <remarks>
    /// This method enables the application to configure the processor affinity for internal Game Chat threads of a
    /// given type.
    /// <para>
    /// For exclusive resource applications, the threads are guaranteed to run on the specified processor. For universal
    /// Windows applications, the specified processor is used as the thread's ideal processor and is only a hint for the
    /// scheduler.
    /// </para>
    /// <para>
    /// This method may be called at any time before or after chat_manager::initialize() and will take effect
    /// immediately. Thread processor settings are persisted across calls to chat_manager::cleanup() and
    /// chat_manager::initialize(). When there are more than 64 cores present, this method always applies to processor
    /// group 0.
    /// </para>
    /// <para>
    /// In order to specify any processor, pass <see cref="c_anyProcessorNumber" /> as the
    /// <paramref name="processorNumber" /> parameter. This is also the default value Game Chat will use if this method
    /// is never called.
    /// </para>
    /// </remarks>
    /// <param name="threadId">The type of internal Game Chat thread to configure processor affinity.</param>
    /// <param name="processorNumber">The zero-based processor number on which the internal Game Chat threads should
    /// run.</param>
    /// <seealso cref="game_chat_thread_id" />
    /// <seealso cref="chat_manager::get_thread_processor" />
    static void set_thread_processor(
        _In_ game_chat_thread_id threadId,
        _In_ uint32_t processorNumber
        ) noexcept;

    /// <summary>
    /// Retrieves the current processor on which internal Game Chat threads will run or are running.
    /// </summary>
    /// <remarks>
    /// This retrieves the current processor affinity for internal Game Chat threads of a given type.
    /// <para>
    /// This method does not require chat_manager::initialize() to have been called first.
    /// </para>
    /// <para>
    /// A reported value of <see cref="c_anyProcessorNumber" /> written to <paramref name="processorNumber" /> indicates
    /// that the thread is free to run on any processor.
    /// </para>
    /// <para>
    /// This call will result in a fatal failure if a prior call to
    /// <see cref="chat_manager::set_thread_affinity_mask" /> has configured the Game Chat internal threads specified
    /// by <param name="threadId"> to run on more than one processor. In that case,
    /// <see cref="chat_manager::get_thread_affinity_mask" /> should be used instead.
    /// </para>
    /// </remarks>
    /// <param name="threadId">The type of internal Game Chat thread for which processor affinity should be
    /// retrieved.</param>
    /// <param name="threadId">The internal thread to retrieve the process number for.</param>
    /// <param name="processorNumber">A place to store the zero-based processor number on which the internal Game Chat
    /// threads will run or are running.</param>
    /// <seealso cref="game_chat_thread_id" />
    /// <seealso cref="chat_manager::set_thread_processor" />
    static void get_thread_processor(
        _In_ game_chat_thread_id threadId,
        _Out_ uint32_t * processorNumber
        ) noexcept;

    /// <summary>
    /// Optionally configures the processors on which internal Game Chat threads will run.
    /// </summary>
    /// <remarks>
    /// This method enables the application to configure the processor affinity for internal Game Chat threads of a
    /// given type. For exclusive resource applications, the threads are guaranteed to run on the specified
    /// processors, of which multiple may be enabled.
    /// </para>
    /// <para>
    /// This method may be called at any time before or after chat_manager::initialize() and will take effect
    /// immediately. Thread processor settings are persisted across calls to chat_manager::cleanup() and
    /// chat_manager::initialize(). When there are more than 64 cores present, this method always applies to processor
    /// group 0.
    /// </para>
    /// <para>
    /// In order to specify any processor, pass <see cref="c_anyProcessor" /> as the
    /// <paramref name="affinityMask" /> parameter. This is also the default value Game Chat will use if this method
    /// is never called.
    /// </para>
    /// </remarks>
    /// <param name="threadId">The type of internal Game Chat thread to configure processor affinity.</param>
    /// <param name="affinityMask">The affinity mask representing on which processor(s) the internal Game Chat threads
    /// should run.</param>
    /// <seealso cref="game_chat_thread_id" />
    /// <seealso cref="chat_manager::get_thread_affinity_mask" />
    static void set_thread_affinity_mask(
        _In_ game_chat_thread_id threadId,
        _In_ uint64_t affinityMask
        ) noexcept;

    /// <summary>
    /// Retrieves the current affinity mask representing the processor(s) on which internal Game Chat threads
    /// will run or are running.
    /// </summary>
    /// <remarks>
    /// This retrieves the current processor affinity for internal Game Chat threads of a given type.
    /// <para>
    /// This method does not require chat_manager::initialize() to have been called first.
    /// </para>
    /// <para>
    /// A reported value of <see cref="c_anyProcessor" /> written to <paramref name="affinityMask" /> indicates
    /// that the thread is free to run on any processor.
    /// </para>
    /// </remarks>
    /// <param name="threadId">The type of internal Game Chat thread for which processor affinity should be
    /// retrieved.</param>
    /// <param name="threadId">The internal thread to retrieve the processor mask for.</param>
    /// <param name="affinityMask">A place to store the affinity mask representing the processor(s) on which
    /// the internal Game Chat threads will run or are running.</param>
    /// <seealso cref="game_chat_thread_id" />
    /// <seealso cref="chat_manager::set_thread_affinity_mask" />
    static void get_thread_affinity_mask(
        _In_ game_chat_thread_id threadId,
        _Out_ uint64_t* affinityMask
        ) noexcept;

    /// <summary>
    /// Optionally configures if legacy ERA and UWP compat mode of Game Chat should be enabled.
    /// </summary>
    /// <remarks>
    /// Enabling legacy ERA and UWP compat mode allows interoperability between Game Core GDK versions of Game Chat and
    /// console ERA and PC UWP versions of Game Chat.
    /// <para>
    /// This method may be called at any time before or after chat_manager::initialize() and will take effect on the
    /// following chat_manager::initialize(). Legacy ERA and UWP compat mode setting is persisted across calls to
    /// chat_manager::cleanup() and chat_manager::initialize().
    /// </para>
    /// <para>
    /// Game Chat utilizes a legacy codec when legacy ERA and UWP compat mode is enabled,
    /// so audio quality and performance may be lower. New titles should not use compatibility mode unless
    /// interoperability with legacy clients is required. This mode may be deprecated and removed in future versions.
    /// </para>
    /// </remarks>
    /// <param name="enabled">Whether compatibility mode should be enabled.</param>
    static void set_legacy_era_uwp_compat_mode_enabled(
        _In_ bool enabled
        ) noexcept;

    /// <summary>
    /// Initializes the object instance.
    /// </summary>
    /// <remarks>
    /// This must be called before any other method, aside from the static methods chat_manager::singleton_instance(),
    /// chat_manager::set_memory_callbacks(), chat_manager::get_memory_callbacks(),
    /// chat_manager::set_thread_processor(), and chat_manager::get_thread_processor(). chat_manager::initialize()
    /// cannot be called again without a subsequent chat_manager::cleanup() call.
    /// <para>
    /// Every call to chat_manager::initialize() should have a corresponding chat_manager::cleanup() call.
    /// </para>
    /// </remarks>
    /// <param name="maxChatUserCount">The maximum total number of local and remote users that will be added to the
    /// local chat_manager instance at any given time. Game Chat pre-allocates memory proportional to the number of
    /// users.</param>
    /// <param name="defaultAudioRenderVolume">The audio render volume that will be used for new users.</param>
    /// <param name="defaultLocalToRemoteCommunicationRelationship">The default communication relationship that will be
    /// used between local and remote users. I.e. when a remote user joins, this will be the default relationship used
    /// between all existing local users and that remote user. When a local user joins, this will be the default
    /// relationship between that local user and all existing remote users.</param>
    /// <param name="sharedDeviceResolutionMode">The resolution mode used for users on a shared device who have
    /// conflicting communication relationships.</param>
    /// <param name="speechToTextConversionMode">Configures the speech-to-text conversion mode.</param>
    /// <param name="audioManipulationMode">
    /// Passing this param as game_chat_audio_manipulation_mode_flags::none allows audio to flow through Game Chat
    /// uninterrupted.
    ///
    /// Passing this param with the game_chat_audio_manipulation_mode_flags::pre_encode_stream_manipulation bit set grants
    /// access to chat user audio data for processing and manipulation on the sender-side. Access to the audio data is
    /// enabled through the use of the <see cref="pre_encode_audio_stream" /> objects. These objects can be queried for using
    /// the <see cref="chat_manager::get_pre_encode_audio_streams" /> method. When Game Chat creates, closes, and destroys
    /// <see cref="pre_encode_audio_stream" /> objects, corresponding <see cref="game_chat_stream_state_change" /> events will
    /// be generated.
    ///
    /// Passing this param with the game_chat_audio_manipulation_mode_flags::post_decode_stream_manipulation bit set grants
    /// access to chat user audio data for processing and manipulation on the receiver-side. Access to the audio data is
    /// enabled through the use of the <see cref="post_decode_audio_source_stream" /> and
    /// <see cref="post_decode_audio_sink_stream" /> objects. These objects can be queried for using the
    /// <see cref="chat_manager::get_post_decode_audio_source_streams" /> and
    /// <see cref="chat_manager::get_post_decode_audio_sink_streams" /> methods. When Game Chat creates, closes, and destroys
    /// <see cref="post_decode_audio_source_stream" /> and <see cref="post_decode_audio_sink_stream" /> objects, corresponding
    /// <see cref="game_chat_stream_state_change" /> events will be generated.
    ///
    /// </param>
    /// <seealso cref="chat_manager::cleanup" />
    /// <seealso cref="chat_manager::singleton_instance" />
    /// <seealso cref="chat_manager::set_memory_callbacks" />
    /// <seealso cref="chat_manager::get_memory_callbacks" />
    /// <seealso cref="chat_manager::set_thread_processor" />
    /// <seealso cref="chat_manager::get_thread_processor" />
    /// <seealso cref="pre_encode_audio_stream" />
    /// <seealso cref="post_decode_audio_source_stream" />
    /// <seealso cref="post_decode_audio_sink_stream" />
    void initialize(
        _In_ uint32_t maxChatUserCount,
        _In_range_(0, 1) float defaultAudioRenderVolume = 1.0f,
        _In_ game_chat_communication_relationship_flags defaultLocalToRemoteCommunicationRelationship =
             game_chat_communication_relationship_flags::none,
        _In_ game_chat_shared_device_communication_relationship_resolution_mode sharedDeviceResolutionMode =
             game_chat_shared_device_communication_relationship_resolution_mode::permissive,
        _In_ game_chat_speech_to_text_conversion_mode speechToTextConversionMode =
             game_chat_speech_to_text_conversion_mode::automatic,
        _In_ game_chat_audio_manipulation_mode_flags audioManipulationMode = game_chat_audio_manipulation_mode_flags::none
        ) noexcept;

    /// <summary>
    /// Immediately reclaims all resources associated with the object.
    /// </summary>
    /// <remarks>
    /// Every call to chat_manager::initialize() should have a corresponding chat_manager::cleanup() call.
    /// </remarks>
    /// <seealso cref="chat_manager::initialize" />
    void cleanup(
        ) noexcept;

    /// <summary>
    /// Configures the audio encoding type and bitrate.
    /// </summary>
    /// <remarks>
    /// If uncalled, the default Game Chat will use is game_chat_audio_encoding_bitrate::kilobits_per_second_24.
    /// <para>
    /// Lower bitrates result in less data transmission but lower audio quality. Higher bitrates result in better audio
    /// quality but more data transmission.
    /// </para>
    /// </remarks>
    /// <param name="audioEncodingBitrate">The encoding bitrate Game Chat should use.</param>
    /// <seealso cref="chat_manager::audio_encoding_bitrate" />
    void set_audio_encoding_bitrate(
        _In_ game_chat_audio_encoding_bitrate audioEncodingBitrate
        ) noexcept;

    /// <summary>
    /// Indicates the audio encoding bitrate.
    /// </summary>
    /// <seealso cref="chat_manager::set_audio_encoding_bitrate" />
    game_chat_audio_encoding_bitrate audio_encoding_bitrate() const noexcept;

    /// <summary>
    /// Retrieves the collection of users that have been added to the local chat_manager instance.
    /// </summary>
    /// <remarks>
    /// The array pointer returned is valid until the next call to chat_manager::add_local_user(),
    /// chat_manager::add_remote_user(), chat_manager::remove_user() or chat_manager::cleanup(). The individual user
    /// objects remain valid until the user has been removed by a call to chat_manager::remove_user().
    /// <param name="chatUserCount">A place to store the number of users that have been added to the local
    /// chat_manager instance.</param>
    /// <param name="chatUsers">A place to store a pointer to the array of users that have been added to the local
    /// chat_manager instance.</param>
    /// </remarks>
    /// <seealso cref="chat_user" />
    void get_chat_users(
        _Out_ uint32_t * chatUserCount,
        _Outptr_result_buffer_(*chatUserCount) chat_user_array * chatUsers
        ) const noexcept;

    /// <summary>
    /// Adds a local user to the local chat_manager instance.
    /// </summary>
    /// <remarks>
    /// The user will be added unconditionally, unless the total number of local and remote users is equal to the max
    /// user count that was indicated in chat_manager::initialize().
    /// <para>
    /// The return value is a pointer to the chat_user object associated with this user. If the Xbox User ID is invalid,
    /// the user's chat_indicator value will be game_chat_user_chat_indicator::platform_restricted.
    /// </para>
    /// <para>
    /// A typical pattern would be:
    /// <code>
    ///     chat_user* newLocalChatUser = chat_manager::singleton_instance().add_local_user(<local_user_xuid>);
    ///
    ///     size_t chatUserCount;
    ///     chat_user_array chatUsers;
    ///     chat_manager::singleton_instance().get_remote_chat_users(&chatUserCount, &chatUsers);
    ///
    ///     for (uint32_t chatUserIndex = 0; chatUserIndex < chatUsercount; ++chatUserIndex)
    ///     {
    ///         chat_user* chatUser = chatUsers[chatUserIndex];
    ///         game_chat_communication_relationship_flags relationship = game_chat_communication_relationship_flags::none;
    ///         if (chatUser can send and receive to/from newLocalChatUser)
    ///         {
    ///             relationship = c_communicationRelationshipSendAndReceiveAll;
    ///         }
    ///         else if (chatUser can send to newLocalChatUser)
    ///         {
    ///             relationship = c_communicationRelationshipSendAll;
    ///         }
    ///         else if (chatUser can receive from newLocalChatUser)
    ///         {
    ///             relationship = c_communicationRelationshipReceiveAll;
    ///         }
    ///
    ///         newLocalChatUser->set_communication_relationship(chatUser, relationship);
    ///     }
    /// </code>
    /// </para>
    /// </remarks>
    /// <param name="xboxUserId">The Xbox User ID of the local user to add.</param>
    /// <seealso cref="chat_user" />
    /// <seealso cref="chat_manager::add_remote_user" />
    /// <seealso cref="remove_user" />
    chat_user * add_local_user(
        _In_ PCWSTR xboxUserId
        ) noexcept;

    /// <summary>
    /// Adds a remote user to the local chat_manager instance.
    /// </summary>
    /// <remarks>
    /// The user will be added unconditionally, unless the total number of local and remote users is equal to the max
    /// user count that was indicated in chat_manager::initialize().
    /// <para>
    /// The return value is a pointer to the chat_user object associated with this user. If the Xbox User ID is invalid,
    /// the user's chat_indicator value will be game_chat_user_chat_indicator::platform_restricted.
    /// </para>
    /// </remarks>
    /// <param name="xboxUserId">The Xbox User ID of the remote user to add.</param>
    /// <param name="locallyUniqueEndpointIdentifier">An identifier generated by the title used to refer to the endpoint
    /// that the remote user is on. And endpoint is a device, such as an Xbox One or a PC. Each endpoint must have a
    /// unique identifier in the local instance of Game Chat; the identifier does not need to be globally unique across
    /// the network. Remote users on the same endpoint, such as two users playing on the same Xbox One, must be added
    /// with the same endpoint identifier. Game Chat will use this identifier to indicate which endpoints data must be
    /// sent to in the game_chat_data_frame structure, and must be used by the title to indicate which endpoint data
    /// came from in chat_manager::process_incoming_data().</param>
    /// <seealso cref="chat_user" />
    /// <seealso cref="chat_manager::add_remote_user" />
    /// <seealso cref="chat_manager::remove_user" />
    /// <seealso cref="chat_manager::process_incoming_data" />
    /// <seealso cref="game_chat_data_frame" />
    chat_user * add_remote_user(
        _In_ PCWSTR xboxUserId,
        _In_ uint64_t locallyUniqueEndpointIdentifier
        ) noexcept;

    /// <summary>
    /// Removes a user, either local or remote, from the local chat_manager instance.
    /// </summary>
    /// <remarks>
    /// This method allows game chat to reclaim resources associated with this user.
    /// <para>
    /// chat_manager::remove_user() must not be called while processing state changes (i.e. after
    /// chat_manager::start_processing_state_changes() has been called and before the corresponding call to
    /// chat_manager::finish_processing_state_changes()). Calling chat_manager::remove_user() while state changes are
    /// being processed can invalidate the memory associated with the removed user, even if a pointer to the memory
    /// associated with that user was returned in a state change. If audio manipulation is enabled, the memory associated
    /// with a removed user will remain valid until the audio manipulation streams associated with the user have been
    /// destroyed.
    /// </para>
    /// </remarks>
    /// <param name="chatUser">The chat_user to remove.</param>
    /// <seealso cref="chat_user" />
    /// <seealso cref="chat_manager::add_remote_user" />
    /// <seealso cref="chat_manager::remove_user" />
    /// <seealso cref="chat_manager::start_processing_state_changes" />
    /// <seealso cref="chat_manager::finish_processing_state_changes" />
    void remove_user(
        _In_ _Post_invalid_ chat_user * chatUser
        ) noexcept;

    /// <summary>
    /// Returns all data frames queued by Game Chat that should title needs to transport to remote Game Chat end points.
    /// </summary>
    /// <remarks>
    /// Calling this less frequently introduces audio latency, but allows Game Chat to coalesce data into large data
    /// frames. Calling this more frequently reduces audio latency, but results in more, smaller data frames.
    /// </remarks>
    /// <seealso cref="chat_manager::finish_processing_data_frames" />
    void start_processing_data_frames(
        _Out_ uint32_t * dataFrameCount,
        _Outptr_result_buffer_(*dataFrameCount) game_chat_data_frame_array * dataFrames
        ) noexcept;

    /// <summary>
    /// This must be called after each chat_manager::start_processing_data_frames() so that Game Chat can reclaim memory
    /// associated with the data frames.
    /// </summary>
    /// <seealso cref="chat_manager::start_processing_data_frames" />
    void finish_processing_data_frames(
        _In_ game_chat_data_frame_array dataFrames
        ) noexcept;

    /// <summary>
    /// Retrieves an array of all Game Chat state changes to process since the last such call.
    /// </summary>
    /// <remarks>
    /// This method provides a list of all changes currently available for the app since the last call to this
    /// method. The app should use the provided array of 0 or more changes to update its own state or UI, and then call
    /// chat_manager::finish_processing_state_changes() with them in a timely manner.
    /// <para>
    /// If you call chat_manager::start_processing_state_changes() again without having called
    /// chat_manager::finish_processing_state_changes() on the list of state change objects returned by a previous call,
    /// this method will throw an exception. You must always call chat_manager::finish_processing_state_changes() with
    /// the values returned by chat_manager::start_processing_state_changes(), even if it returns 0 state change
    /// entries.
    /// </para>
    /// <para>
    /// Once chat_manager::start_processing_state_changes() has been called, chat_manager::remove_user() must not be
    /// called until after the corresponding call to chat_manager::finish_processing_state_changes(). Calling
    /// chat_manager::remove_user() while state changes are being processed will invalidate the memory associated with
    /// the removed user, even if a pointer to the memory associated with that user was returned in a state change.
    /// </para>
    /// </remarks>
    /// <param name="stateChangeCount">A place to write the number of game_chat_state_change entries for the app to
    /// handle in the <paramref name="stateChanges" /> array.</param>
    /// <param name="stateChanges">A place to store a pointer to the array of all game_chat_state_change entries for the
    /// app to handle and then pass to chat_manager::finish_processing_state_changes().</param>
    /// <seealso cref="game_chat_state_change" />
    /// <seealso cref="game_chat_state_change_array" />
    /// <seealso cref="chat_manager::finish_processing_state_changes" />
    /// <seealso cref="chat_manager::remove_user"/ >
    void start_processing_state_changes(
        _Out_ uint32_t * stateChangeCount,
        _Outptr_result_buffer_(*stateChangeCount) game_chat_state_change_array * stateChanges
        ) noexcept;

    /// <summary>
    /// Returns an array of Game Chat state changes that were being processed.
    /// </summary>
    /// <remarks>
    /// This method informs the Game Chat library that all state changes reported by a previous call to
    /// chat_manager::start_processing_state_changes() have now been handled by the app, so their associated resources
    /// can be reclaimed. You must call chat_manager::finish_processing_state_changes() with the same pointer returned
    /// by chat_manager::start_processing_state_changes() before you're permitted to call
    /// chat_manager::start_processing_state_changes() again. You must do so even if 0 state change entries had been returned.
    /// <para>
    /// For best results, you should minimize the time you spend handling state changes before calling
    /// chat_manager::finish_processing_state_changes().
    /// </para>
    /// </remarks>
    /// <param name="stateChanges">The pointer to the array of changes previously returned by
    /// chat_manager::start_processing_state_changes() that have now been handled by the app.</param>
    /// <seealso cref="game_chat_state_change" />
    /// <seealso cref="game_chat_state_change_array" />
    /// <seealso cref="chat_manager::start_processing_state_changes" />
    void finish_processing_state_changes(
        _In_ game_chat_state_change_array stateChanges
        ) noexcept;

    /// <summary>
    /// Used to give Game Chat data that originated from a remote instance of Game Chat.
    /// </summary>
    /// <remarks>
    /// When the app receives data that originated from a remote instance of Game Chat, this method should be called to
    /// deliver the data to the local instance of Game Chat.
    /// </remarks>
    /// <param name="sourceEndpointIdentifier">The identifier associated with an endpoint that was advertised to Game
    /// Chat with a previous call to add_remote_user().</param>
    /// <param name="bufferByteCount">The size of the data buffer, in bytes.</param>
    /// <param name="buffer">The data buffer.</param>
    /// <seealso cref="game_chat_data_frame" />
    /// <seealso cref="chat_manager::add_remote_user" />
    void process_incoming_data(
        _In_ uint64_t sourceEndpointIdentifier,
        _In_ uint32_t bufferByteCount,
        _In_reads_(bufferByteCount) const void * buffer
        ) noexcept;

    /// <summary>
    /// Indicates the audio manipulation mode enabled for this chat instance.
    /// </summary>
    /// <seealso cref="chat_manager::initialize" />
    /// <seealso cref="game_chat_audio_manipulation_mode_flags" />
    game_chat_audio_manipulation_mode_flags audio_manipulation_mode(
        ) noexcept;

    /// <summary>
    /// Gets the collection of pre-encode audio streams.
    /// </summary>
    /// <remarks>
    /// The streams returned by this method are not in any guaranteed order. The order of the streams is also not
    /// guaranteed to be consistent between calls to this method.
    /// The array returned is only valid until a
    /// <see cref="game_chat_stream_state_change_type::pre_encode_audio_stream_destroyed" /> stream state change is returned.
    /// Will only return streams if pre-encode audio stream processing has been enabled.
    /// </remarks>
    /// <seealso cref="chat_manager::get_post_decode_audio_source_streams" />
    /// <seealso cref="chat_manager::get_post_decode_audio_sink_streams" />
    void get_pre_encode_audio_streams(
        _Out_ uint32_t* preEncodeAudioStreamCount,
        _Outptr_result_buffer_(*preEncodeAudioStreamCount) pre_encode_audio_stream_array* preEncodeAudioStreams
        ) noexcept;

    /// <summary>
    /// Gets the collection of post-decode audio source streams.
    /// </summary>
    /// <remarks>
    /// The streams returned by this method are not in any guaranteed order. The order of the streams is also not
    /// guaranteed to be consistent between calls to this method.
    /// The array returned is only valid until a
    /// <see cref="game_chat_stream_state_change_type::post_decode_audio_source_stream_destroyed" /> stream state change is
    /// returned.
    /// Will only return streams if remote audio stream processing has been enabled.
    /// </remarks>
    /// <seealso cref="chat_manager::get_pre_encode_audio_streams" />
    /// <seealso cref="chat_manager::get_post_decode_audio_sink_streams" />
    void get_post_decode_audio_source_streams(
        _Out_ uint32_t* postDecodeAudioSourceStreamCount,
        _Outptr_result_buffer_(*postDecodeAudioSourceStreamCount) post_decode_audio_source_stream_array* postDecodeAudioSourceStreams
        ) noexcept;

    /// <summary>
    /// Gets the collection of post-decode audio sink streams.
    /// </summary>
    /// <remarks>
    /// The streams returned by this method are not in any guaranteed order. The order of the streams is also not
    /// guaranteed to be consistent between calls to this method.
    /// The array returned is only valid until a
    /// <see cref="game_chat_stream_state_change_type::post_decode_audio_sink_stream_destroyed" /> stream state change is
    /// returned.
    /// Will only return streams if remote audio stream processing has been enabled.
    /// </remarks>
    /// <seealso cref="chat_manager::get_pre_encode_audio_streams" />
    /// <seealso cref="chat_manager::get_post_decode_audio_source_streams" />
    void get_post_decode_audio_sink_streams(
        _Out_ uint32_t* postDecodeAudioSinkStreamCount,
        _Outptr_result_buffer_(*postDecodeAudioSinkStreamCount) post_decode_audio_sink_stream_array* postDecodeAudioSinkStreams
        ) noexcept;

    /// <summary>
    /// Retrieves an array of all Game Chat audio stream state changes since the last call. The streams accompanying
    /// these changes should be processed for any necessary initialization or cleanup and then returned to
    /// <see cref="chat_manager::finish_processing_stream_state_changes" />.
    /// </summary>
    /// <remarks>
    /// This method should not be called on your UI thread because it will require coordination with your audio thread.
    /// </remarks>
    /// <param name="streamStateChangeCount">
    /// A place to write the number of game_chat_stream_state_change entries for the app to handle in the
    /// <paramref name="streamStateChanges" /> array.
    /// </param>
    /// <param name="streamStateChanges">
    /// A place to store a pointer to the array of all stream state change entries for the app to handle and then
    /// pass to <see cref="chat_manager::finish_processing_stream_state_changes" />.
    /// </param>
    /// <seealso cref="game_chat_stream_state_change" />
    /// <seealso cref="game_chat_stream_state_change_array" />
    /// <seealso cref="chat_manager::finish_processing_stream_state_changes" />
    void start_processing_stream_state_changes(
        _Out_ uint32_t* streamStateChangeCount,
        _Outptr_result_buffer_(*streamStateChangeCount) game_chat_stream_state_change_array* streamStateChanges
        ) noexcept;

    /// <summary>
    /// Returns an array of Game Chat stream state changes that were processed by the caller of
    /// <see cref="chat_manager::start_processing_stream_state_changes" />.
    /// </summary>
    /// <remarks>
    /// This method informs the Game Chat library that all stream state changes reported by a previous call to
    /// <see cref="chat_manager::start_processing_stream_state_changes" /> have now been handled by the app. You must
    /// call <see cref="chat_manager::finish_processing_stream_state_changes" /> with the same pointer returned
    /// by <see cref="chat_manager::start_processing_stream_state_changes" /> before you're permitted to call
    /// <see cref="chat_manager::start_processing_stream_state_changes()" /> again. You must do so even if 0 state
    /// change entries had been returned.
    /// <para>
    /// For best results, you should minimize the time you spend handling stream state changes before calling
    /// <see cref="chat_manager::finish_processing_stream_state_changes" />.
    /// </para>
    /// </remarks>
    /// <param name="streamStateChanges">
    /// The pointer to the array of changes previously returned by
    /// <see cref="chat_manager::start_processing_stream_state_changes" /> that have now been handled by the app.
    /// </param>
    /// <seealso cref="game_chat_stream_state_change" />
    /// <seealso cref="game_chat_stream_state_change_array" />
    /// <seealso cref="chat_manager::start_processing_stream_state_changes" />
    void finish_processing_stream_state_changes(
        _In_ game_chat_stream_state_change_array streamStateChanges
        ) noexcept;

private:
    chat_manager();
    ~chat_manager();
    chat_manager(const chat_manager&) = delete;
    chat_manager& operator=(const chat_manager&) = delete;
    chat_manager(const chat_manager&&) = delete;
    chat_manager& operator=(const chat_manager&&) = delete;
};

} // end namespace game_chat_2
} } // end namespace xbox::services
