#pragma once
#include <string>
#include <vector>
#include <map>
#include <spdlog/spdlog.h>

#include "smf.hpp"

namespace ORGame
{

    enum class Difficulty
    {
        NONE,
        Easy,
        Medium,
        Hard,
        Expert
    };

    enum class TrackType
    {
        NONE,
        Guitar,
        Bass,
        Drums,
        Vocals,
        // Non-player track types
        Events
    };

    enum class NoteType
    {
        NONE,
        Green,
        Red,
        Yellow,
        Blue,
        Orange,
    };

    enum class TempoEventType
    {
        Note,
        Tempo,
        TimeSignature,
        Bar,
    };

    struct MidiNoteDefinition
    {
        int green;
        int red;
        int yellow;
        int blue;
        int orange;
        int power;
    };

    struct TrackNote
    {
        NoteType type;
        double time;
        double length;
        int objNoteID;
        int objTailID;
        bool played;
    };

    struct TempoEvent
    {
        int numerator;
        int denominator;
        int qnScaleFactor;
        int qnLength;
        double time;
    };

    enum class BarType
    {
        measure,
        beat,
        upbeat, // This isn't always half of a beat one example of this is: 6/8 compound time
    };

    struct BarEvent
    {
        BarType type;
        // int count;
        double time;
    };

    enum class EventType
    {
        solo,
        drive,
        freestyle,
    };

    // Events are for things that have a position/length with no special data accociated with them
    // So drive/solo/freestyle are some examples.
    struct Event
    {
        EventType type;
        double time;
        double length;
    };


    struct TrackInfo
    {
        TrackType type;
        Difficulty difficulty;
    };

    class TempoTrack
    {
    public:
        void add_tempo_event(int ppqn, double time);
        void add_time_sig_event(int numerator, int denominator, int compoundFactor, double time);

        std::vector<TempoEvent*> get_events(double start, double end);
        std::vector<TempoEvent*> get_events();

        void mark_bars();
        std::vector<BarEvent*> get_bars(double start, double end);
        std::vector<BarEvent*> get_bars();

    private:
    	std::vector<TempoEvent> m_tempo;
        std::vector<BarEvent> m_bars;
    };


    class Track
    {
    public:

        Track(TrackInfo info);

        TrackInfo info();

        void add_note(NoteType type, double time, bool on);
        std::vector<TrackNote*> get_notes_in_frame(double start, double end);
        std::vector<TrackNote*> get_notes();

        void set_event(EventType type, double time, bool on);
        std::vector<Event> *get_events();

    private:
        TrackInfo m_info;
        std::vector<TrackNote> m_notes;
        std::vector<Event> m_events;
    };

    class Song
    {
    public:
        Song(std::string songpath);
        void add(TrackType type, Difficulty difficulty);
        bool load();
        void load_track(TrackInfo& trackInfo);
        void load_tracks();
        std::vector<Track> *get_tracks();
        std::vector<TrackInfo> *get_track_info();
        TempoTrack *get_tempo_track();

        double length();

    private:
        ORCore::SmfReader m_midi;
        std::vector<TrackInfo> m_tracksInfo;
        std::vector<Track> m_tracks;
        TempoTrack m_tempoTrack;
        std::string m_path;
        double m_length;

    };

    // Functions are mainly used within the Song class
    const std::string diff_type_to_name(Difficulty diff);
    const TrackType get_track_type(std::string trackName);
    const std::string track_name_to_type(TrackType type);
} // namespace ORGame
