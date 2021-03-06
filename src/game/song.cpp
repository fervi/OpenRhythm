#include "config.hpp"
#include <stdexcept>
#include <algorithm>
#include <iostream>
#include "song.hpp"

#include "vfs.hpp"

namespace ORGame
{


    static std::shared_ptr<spdlog::logger> logger;

    /////////////////////////////////////
    // TempoTrack Class methods
    /////////////////////////////////////

    void TempoTrack::add_tempo_event(int qnLength, double time)
    {
        // If this is the first change in tempo/timesignature create an event with the other half of state
        if (m_tempo.size() == 0)
        {
            m_tempo.push_back({0, 0, 0, qnLength, time});
        }
        else
        {
            // If the previous event is located at the same time as the current one overwrite the data to reflect the current state.
            auto &eventLast = m_tempo.back();
            if (eventLast.time == time)
            {
                eventLast.qnLength = qnLength;
            }
            else
            {
                // Otherwise we copy the previous change and update the data to reflect the new state.
                TempoEvent tempoEvent = eventLast;
                tempoEvent.time = time;
                tempoEvent.qnLength = qnLength;
                m_tempo.push_back(tempoEvent);

            }
        }
    }

    void TempoTrack::add_time_sig_event(int numerator, int denominator, int compoundFactor, double time)
    {
        // If this is the first change in tempo/timesignature create an event with the other half of state
        if (m_tempo.size() == 0)
        {
            m_tempo.push_back({numerator, denominator, compoundFactor, 0, time});
        }
        else
        {
            // If the previous event is located at the same time as the current one overwrite the data to reflect the current state.
            auto &eventLast = m_tempo.back();
            if (eventLast.time == time)
            {
                eventLast.numerator = numerator;
                eventLast.denominator = denominator;
                eventLast.qnScaleFactor = compoundFactor;
            }
            else
            {
                // Otherwise we copy the previous change and update the data to reflect the new state.
                TempoEvent tempoEvent = eventLast;
                tempoEvent.time = time;
                tempoEvent.numerator = numerator;
                tempoEvent.denominator = denominator;
                tempoEvent.qnScaleFactor = compoundFactor;

                m_tempo.push_back(tempoEvent);
            }
        }
    }

    std::vector<TempoEvent*> TempoTrack::get_events(double start, double end)
    {
        std::vector<TempoEvent*> events;
        for (auto &tempo : m_tempo) {
            if (tempo.time >= start && tempo.time <= end) {
                events.push_back(&tempo);
            }
        }
        return events;
    }

    std::vector<TempoEvent*> TempoTrack::get_events()
    {
        std::vector<TempoEvent*> events;
        for (auto &tempo : m_tempo) {
            events.push_back(&tempo);
        }
        return events;
    }

    void TempoTrack::mark_bars()
    {
        TempoEvent *currentTempo = nullptr;
        double beatSubdivision = 1.0; // How many times to subdivide the beat
        double measureCount = 0.0;
        double beatTsFactor = 1.0;
        double incr = 0.0;
        int interMeasureBeatCount = 0;
        bool timeSigChanged = true;
        for (auto &nextTempo : m_tempo)
        {

            if (currentTempo == nullptr)
            {
                currentTempo = &nextTempo;
                std::cout << "Tempo change info" << currentTempo->numerator << " " << currentTempo->denominator << " " << currentTempo->qnLength << std::endl;
                continue;
            }

            beatTsFactor = 4.0/currentTempo->denominator;

            incr = (currentTempo->qnLength / (beatSubdivision*1'000'000.0)) * beatTsFactor;

            double beats = ((nextTempo.time - currentTempo->time) / incr);

            double measures = ( beats / currentTempo->numerator);

            std::cout << "Measures in change: " << measures << " beats " << beats << std::endl;

            if (measures < 1)
            {
                std::cout << "Tempo change info" << currentTempo->numerator << " " << currentTempo->denominator << " " << currentTempo->qnLength << std::endl;
            }

            // One thing that needs to happen here, if we have tempo changes that are less than a beat we need to accumulate the time between them
            // and create the next beat at that point. Beat lines should be always generated from the previous beat's position. However to generate
            // long stretches of beat lines we should generate those based on multiplication to reduce rounding errors.
            // To make that work we cannot base the multiplication based on the current tempo position, but instead tempoMarkingPeriodBegin.
            // tempoMarkingPeriodBegin will be the position of the last committed beat line.
            // a beat line will only be committed to tempoMarkingPeriodBegin when we have reached the last whole markable beat in a time period.
            // The remaining time between this final beat, and the next tempo change will be rolled over into the next beat period.
            // We need to forget thinking in terms of "beats" and instead think in terms of time. We cannot lose time.

            measureCount = static_cast<int>(((nextTempo.time - currentTempo->time) / incr) / currentTempo->numerator);

            for (int i=0; i < static_cast<int>(beats)+1; i++)
            {
                if (interMeasureBeatCount == 0)
                {
                    m_bars.push_back({BarType::measure, currentTempo->time + (incr*i)});
                } else {
                    m_bars.push_back({BarType::beat, currentTempo->time + (incr*i)});
                }

                interMeasureBeatCount++;
                if (interMeasureBeatCount >= currentTempo->numerator)
                {
                    interMeasureBeatCount = 0;
                }

            }

            if (currentTempo->numerator != nextTempo.numerator || currentTempo->denominator != nextTempo.denominator)
            {
                std::cout << "TS CHANGED MARKING" << std::endl;
                interMeasureBeatCount = 0;
            }

            currentTempo = &nextTempo;
        }
    }

    std::vector<BarEvent*> TempoTrack::get_bars(double start, double end)
    {
        std::vector<BarEvent*> events;
        for (auto &bar : m_bars) {
            if (bar.time >= start && bar.time <= end) {
                events.push_back(&bar);
            }
        }
        return events;
    }

    std::vector<BarEvent*> TempoTrack::get_bars()
    {
        std::vector<BarEvent*> events;
        for (auto &bar : m_bars) {
            events.push_back(&bar);
        }
        return events;
    }

    /////////////////////////////////////
    // Track Class
    /////////////////////////////////////

    Track::Track(TrackInfo info)
    : m_info(info)
    {
        logger = spdlog::get("default");
    }


    TrackInfo Track::info() {
        return m_info;
    }

    void Track::add_note(NoteType type, double time, bool on)
    {
        static std::vector<std::pair<NoteType, int>> activeNotes;

        if (on) {
            int index = m_notes.size();
            activeNotes.emplace_back(type, index);
            m_notes.push_back({type, time, 0.0});
        } else {
            auto findFunc = [&](const auto& element)
            {
                return element.first == type;
            };
            auto item = std::find_if( activeNotes.begin(), activeNotes.end(), findFunc);
            if (item != activeNotes.end())
            {
                auto &note = m_notes[item->second];
                note.length = time - note.time;
                activeNotes.erase(item);
            }
        }
    }

    void Track::set_event(EventType type, double time, bool on)
    {
        static std::vector<std::pair<EventType, int>> activeEvents;

        if (on) {
            int index = m_events.size();
            activeEvents.emplace_back(type, index);
            m_events.push_back({type, time, 0.0});
        } else {
            auto findFunc = [&](const auto& element)
            {
                return element.first == type;
            };
            auto item = std::find_if( activeEvents.begin(), activeEvents.end(), findFunc);
            if (item != activeEvents.end())
            {
                auto &event = m_events[item->second];
                event.length = time - event.time;
                activeEvents.erase(item);
            }

        }
    }

    std::vector<Event> *Track::get_events()
    {
        return &m_events;
    }

    // void set_

    std::vector<TrackNote*> Track::get_notes_in_frame(double start, double end)
    {   
        std::vector<TrackNote*> notes;
        for ( auto &note : m_notes) {
            if (note.time >= start && note.time <= end) {
                notes.emplace_back( &note );
            }
        }
        return notes;
    }

    std::vector<TrackNote*> Track::get_notes()
    {   
        std::vector<TrackNote*> notes;
        for ( auto &note : m_notes) {
            notes.emplace_back( &note );
        }
        return notes;
    }

    using MidiNoteMap = std::map<int, NoteType>;
    const int solo_marker = 0x67;
    const int drive_marker = 0x74;
    const std::map<Difficulty, MidiNoteMap> midiDiffMap {
        {Difficulty::Expert, {
                {0x60, NoteType::Green},
                {0x61, NoteType::Red},
                {0x62, NoteType::Yellow},
                {0x63, NoteType::Blue},
                {0x64, NoteType::Orange}
            }
        },
        {Difficulty::Hard, {
                {0x54, NoteType::Green},
                {0x55, NoteType::Red},
                {0x56, NoteType::Yellow},
                {0x57, NoteType::Blue},
                {0x58, NoteType::Orange}
            }
        },
        {Difficulty::Medium, {
                {0x48, NoteType::Green},
                {0x49, NoteType::Red},
                {0x4a, NoteType::Yellow},
                {0x4b, NoteType::Blue},
                {0x4c, NoteType::Orange}
            }
        },
        {Difficulty::Easy, {
                {0x3c, NoteType::Green},
                {0x3d, NoteType::Red},
                {0x3e, NoteType::Yellow},
                {0x3f, NoteType::Blue},
                {0x40, NoteType::Orange}
            }
        }
    };

    const std::map<Difficulty, std::string> diffNameMap {
        {Difficulty::Expert, "Expert"},
        {Difficulty::Hard, "Hard"},
        {Difficulty::Medium, "Medium"},
        {Difficulty::Easy, "Easy"}
    };

    const std::map<TrackType, std::string> trackNameMap {
        {TrackType::Guitar, "Guitar"},
        {TrackType::Bass, "Bass"},
        {TrackType::Drums, "Drums"},
        {TrackType::Vocals, "Vocals"},
        {TrackType::Events, "Events"},
        {TrackType::NONE, "None"}
    };

    const std::map<std::string, TrackType> midiTrackTypeMap {
        {"PART GUITAR", TrackType::Guitar},
        {"PART BASS", TrackType::Bass},
        {"PART DRUMS", TrackType::Drums},
        {"PART VOCALS", TrackType::Vocals},
        {"EVENTS", TrackType::Events},
        {"", TrackType::NONE}
    };

    // Convenence functions for accessing the maps.
    const std::string diff_type_to_name(Difficulty diff)
    {
        try {
            return diffNameMap.at(diff);
        } catch (std::out_of_range &err) {
            return "";
        }
    }

    const std::string track_type_to_name(TrackType type)
    {
        try {
            return trackNameMap.at(type);
        } catch (std::out_of_range &err) {
            return "";
        }
    }

    const TrackType get_track_type(std::string trackName)
    {
        try {
            return midiTrackTypeMap.at(trackName);
        } catch (std::out_of_range &err) {
            return TrackType::NONE;
        }
    }

    /////////////////////////////////////
    // Song Class methods
    /////////////////////////////////////

    Song::Song(std::string songpath)
    : m_path(songpath),
    m_midi("notes.mid")
    {
        logger = spdlog::get("default");
    }

    void Song::add( TrackType type, Difficulty difficulty )
    {
        if ( type < TrackType::Events ) {
            m_tracksInfo.push_back( {type, difficulty} );
        }
    }

    bool Song::load()
    {
        const ORCore::TempoTrack &tempoTrack = *m_midi.get_tempo_track();

        for (auto &eventOrder : tempoTrack.tempoOrdering)
        {

            if (eventOrder.type == ORCore::TtOrderType::TimeSignature)
            {
                auto &ts = tempoTrack.timeSignature[eventOrder.index];
                m_tempoTrack.add_time_sig_event(ts.numerator, ts.denominator, ts.thirtySecondPQN/8.0, m_midi.pulsetime_to_abstime(ts.info.info.pulseTime));
                logger->trace(_("Time signature change recieved at time {} {}/{}"), m_midi.pulsetime_to_abstime(ts.info.info.pulseTime), ts.numerator, ts.denominator);
            }
            else if (eventOrder.type == ORCore::TtOrderType::Tempo)
            {
                auto &tempo = tempoTrack.tempo[eventOrder.index];
                m_tempoTrack.add_tempo_event(tempo.qnLength, tempo.absTime);
                logger->trace(_("Tempo change recieved at time {} {}"), tempo.absTime, tempo.qnLength);
            } 
        }

        m_tempoTrack.mark_bars();

        std::vector<ORCore::SmfTrack*> midiTracks = m_midi.get_tracks();

        bool foundUsable = false;
        
        for (auto midiTrack : midiTracks)
        {
            TrackType type = get_track_type(midiTrack->name);
            if (type == TrackType::Guitar)
            {
                // Add all difficulties for this track
                add(type, Difficulty::Expert);
                add(type, Difficulty::Hard);
                add(type, Difficulty::Medium);
                add(type, Difficulty::Easy);
                foundUsable = true;
            }
        }
        if (!foundUsable)
        {
            throw std::runtime_error("Invalid song format.");
        }

        logger->debug(_("Song loaded"));

        return false;
    }

    void Song::load_track(TrackInfo& trackInfo)
    {
        Track track(trackInfo);

        logger->debug(_("Loading Track {} {}"), track_type_to_name(trackInfo.type), diff_type_to_name(trackInfo.difficulty));

        std::vector<ORCore::SmfTrack*> midiTracks = m_midi.get_tracks();

        const MidiNoteMap &noteMap = midiDiffMap.at(trackInfo.difficulty);

        NoteType note;

        for (auto midiTrack : midiTracks)
        {
            TrackType type = get_track_type(midiTrack->name);
            if (type == trackInfo.type)
            {

                for (auto &midiEvent : midiTrack->midiEvents)
                {
                    // TODO - Clean this up this WILL get very messy eventually.
                    if (midiEvent.message == ORCore::NoteOn) {
                        if (midiEvent.data1 == solo_marker) {
                            if (midiEvent.data2 != 0)
                            {
                                track.set_event(EventType::solo, m_midi.pulsetime_to_abstime(midiEvent.info.pulseTime), true);
                            } else {
                                track.set_event(EventType::solo, m_midi.pulsetime_to_abstime(midiEvent.info.pulseTime), false);

                            }
                        } else if (midiEvent.data1 == drive_marker) {
                            if (midiEvent.data2 != 0)
                            {
                                track.set_event(EventType::drive, m_midi.pulsetime_to_abstime(midiEvent.info.pulseTime), true);
                            } else {
                                track.set_event(EventType::drive, m_midi.pulsetime_to_abstime(midiEvent.info.pulseTime), false);
                            }
                        } else {
                            try {
                                note = noteMap.at(midiEvent.data1);
                            } catch (std::out_of_range &err) {
                                continue;
                            }
                            if (note != NoteType::NONE) {
                                if (midiEvent.data2 != 0)
                                {
                                    track.add_note(note, m_midi.pulsetime_to_abstime(midiEvent.info.pulseTime), true);
                                } else {
                                    track.add_note(note, m_midi.pulsetime_to_abstime(midiEvent.info.pulseTime), false);
                                }
                            }
                        }
                    } else if (midiEvent.message == ORCore::NoteOff) {
                        if (midiEvent.data1 == solo_marker) {
                            track.set_event(EventType::solo, m_midi.pulsetime_to_abstime(midiEvent.info.pulseTime), false);
                        } else if (midiEvent.data1 == drive_marker) {
                            track.set_event(EventType::drive, m_midi.pulsetime_to_abstime(midiEvent.info.pulseTime), false);
                        } else {
                            try {
                                note = noteMap.at(midiEvent.data1);
                            } catch (std::out_of_range &err) {
                                continue;
                            }
                            if (note != NoteType::NONE) {
                                track.add_note(note, m_midi.pulsetime_to_abstime(midiEvent.info.pulseTime), false);
                            }
                        }
                    }

                }
                m_length = midiTrack->endTime;
                break;
            }
        }
        m_tracks.push_back(track);
    }

    // Load all tracks
    void Song::load_tracks()
    {
        for (auto &trackInfo : m_tracksInfo)
        {
            load_track(trackInfo);
        }
        logger->debug(_("{} Tracks processed"), m_tracks.size());
    }

    std::vector<Track> *Song::get_tracks()
    {
        return &m_tracks;
    }

    std::vector<TrackInfo> *Song::get_track_info()
    {
        return &m_tracksInfo; 
    }

    TempoTrack *Song::get_tempo_track()
    {
        return &m_tempoTrack;
    };

    double Song::length()
    {
        return m_length;
    };

} // namespace ORGame
