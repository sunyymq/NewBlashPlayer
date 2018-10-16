//
// Created by parallels on 9/9/18.
//

#ifndef NBMEDIASOURCE_H
#define NBMEDIASOURCE_H

#include "foundation/NBMetaData.h"
#include "NBMediaBuffer.h"

class NBMediaSource {
public:
    NBMediaSource();
    virtual ~NBMediaSource();

public:
    // To be called before any other methods on this object, except
    // getFormat().
    virtual nb_status_t start(NBMetaData *params = NULL) = 0;

    // Any blocking read call returns immediately with a result of NO_INIT.
    // It is an error to call any methods other than start after this call
    // returns. Any buffers the object may be holding onto at the time of
    // the stop() call are released.
    // Also, it is imperative that any buffers output by this object and
    // held onto by callers be released before a call to stop() !!!
    virtual nb_status_t stop() = 0;

    // Returns the format of the data output by this media source.
    virtual NBMetaData* getFormat() = 0;

    struct ReadOptions;

    // Returns a new buffer of data. Call blocks until a
    // buffer is available, an error is encountered of the end of the stream
    // is reached.
    // End of stream is signalled by a result of ERROR_END_OF_STREAM.
    // A result of INFO_FORMAT_CHANGED indicates that the format of this
    // MediaSource has changed mid-stream, the client can continue reading
    // but should be prepared for buffers of the new configuration.
    virtual nb_status_t read(
            NBMediaBuffer **buffer, ReadOptions *options = NULL) = 0;
    
    // Discard nonref ref and looperfilter if video is far delay audio
    virtual void discardNonRef() {
    }
    
    // restore the default discard state
    virtual void restoreDiscard() {
    }

public:
    // Options that modify read() behaviour. The default is to
    // a) not request a seek
    // b) not be late, i.e. lateness_us = 0
    struct ReadOptions {
        enum SeekMode {
            SEEK_PREVIOUS_SYNC,
            SEEK_NEXT_SYNC,
            SEEK_CLOSEST_SYNC,
            SEEK_CLOSEST,
        };

        ReadOptions();

        // Reset everything back to defaults.
        void reset();

        void setSeekTo(int64_t time_us, SeekMode mode = SEEK_CLOSEST_SYNC);
        void clearSeekTo();
        bool getSeekTo(int64_t *time_us, SeekMode *mode) const;

        void setLateBy(int64_t lateness_us);
        int64_t getLateBy() const;

    private:
        enum Options {
            kSeekTo_Option      = 1,
        };

        uint32_t mOptions;
        int64_t mSeekTimeUs;
        SeekMode mSeekMode;
        int64_t mLatenessUs;
    };
};

#endif //NBMEDIASOURCE_H
