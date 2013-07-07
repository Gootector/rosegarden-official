/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2013 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_COMPOSITIONMODELIMPL_H
#define RG_COMPOSITIONMODELIMPL_H

#include "base/Selection.h"
#include "base/SnapGrid.h"
#include "CompositionRect.h"
#include "CompositionItem.h"
#include "SegmentOrderer.h"
#include "base/Event.h"

#include <map>
#include <set>

#include <QColor>
#include <QPoint>

#include <QRect>
#include <vector>

class RectRanges;
class AudioPreviewDrawData;
class AudioPreviewData;


namespace Rosegarden
{

class Studio;
class Segment;
class RulerScale;
class Event;
class Composition;
class AudioPreviewUpdater;
class AudioPreviewThread;

typedef std::vector<QImage> PixmapArray;

/// Composition User Interface Controller.
/**
 * In the Entity/Boundary/Control model, this is (roughly) a control class.
 * Composition is the associated entity (problem domain object) class, while
 * CompositionView is the associated boundary (interface) class.
 *
 * This class might be renamed CompositionUIController.
 */
class CompositionModelImpl : public QObject, public CompositionObserver, public SegmentObserver
{
    Q_OBJECT
public:

    struct CompositionItemCompare {
        bool operator()(const CompositionItem &c1, const CompositionItem &c2) const;
    };

    typedef std::vector<QRect> rectlist;
    // ??? rename: ycoordlist  This is not a height list.
    typedef std::vector<int> heightlist;
    typedef std::vector<CompositionRect> rectcontainer;
    typedef std::set<CompositionItem, CompositionItemCompare> itemcontainer;

    struct AudioPreviewDrawDataItem {
        AudioPreviewDrawDataItem(PixmapArray p, QPoint bp, QRect r) :
            pixmap(p), basePoint(bp), rect(r), resizeOffset(0) {};
        PixmapArray pixmap;
        QPoint basePoint;
        QRect rect;

        // when showing a segment that is being resized from the
        // beginning, this contains the offset between the current
        // rect of the segment and the resized one
        int resizeOffset;
    };

    typedef std::vector<AudioPreviewDrawDataItem> AudioPreviewDrawData;

    struct RectRange {
        std::pair<rectlist::iterator, rectlist::iterator> range;
        QPoint basePoint;
        QColor color;
    };

    typedef std::vector<RectRange> RectRanges;

    class AudioPreviewData {
    public:
        AudioPreviewData(bool showMinima, unsigned int channels) : m_showMinima(showMinima), m_channels(channels) {};
        // ~AudioPreviewData();

        bool showsMinima()              { return m_showMinima; }
        void setShowMinima(bool s)      { m_showMinima = s;    }

        unsigned int getChannels()       { return m_channels;   }
        void setChannels(unsigned int c) { m_channels = c;      }

        const std::vector<float> &getValues() const { return m_values;  }
        void setValues(const std::vector<float>&v) { m_values = v; }

        QRect getSegmentRect()              { return m_segmentRect; }
        void setSegmentRect(const QRect& r) { m_segmentRect = r; }

    protected:
        std::vector<float> m_values;
        bool               m_showMinima;
        unsigned int       m_channels;
        QRect              m_segmentRect;

    private:
        // no copy ctor
        AudioPreviewData(const AudioPreviewData&);
    };

    enum ChangeType { ChangeMove, ChangeResizeFromStart, ChangeResizeFromEnd };

    CompositionModelImpl(Composition& compo,
                         Studio& studio,
                         RulerScale *rulerScale,
                         int vStep);

    virtual ~CompositionModelImpl();

    virtual unsigned int getNbRows();
    virtual const rectcontainer& getRectanglesIn(const QRect& rect,
                                                 RectRanges* notationRects, AudioPreviewDrawData* audioRects);
    virtual heightlist getTrackDividersIn(const QRect& rect);
    virtual itemcontainer getItemsAt (const QPoint&);
    virtual timeT getRepeatTimeAt (const QPoint&, const CompositionItem&);

    virtual SnapGrid& grid() { return m_grid; }

    virtual void setPointerPos(int xPos);
    virtual void setSelected(const CompositionItem&, bool selected = true);
    virtual bool isSelected(const CompositionItem&) const;
    virtual void setSelected(const itemcontainer&);
    virtual void setSelected(const Segment*, bool selected = true);
    virtual bool isSelected(const Segment*) const;
    virtual void clearSelected();
    virtual bool haveSelection() const { return !m_selectedSegments.empty(); }
    virtual bool haveMultipleSelection() const { return m_selectedSegments.size() > 1; }
    virtual void signalSelection();
    virtual void setSelectionRect(const QRect&);
    virtual void finalizeSelectionRect();
    virtual QRect getSelectionContentsRect();
    virtual void signalContentChange();

    virtual void addRecordingItem(const CompositionItem&);
    virtual void removeRecordingItem(const CompositionItem &);
    virtual void clearRecordingItems();
    virtual bool haveRecordingItems() { return !m_recordingSegments.empty(); }

    virtual void startChange(const CompositionItem&, ChangeType change);
    virtual void startChangeSelection(ChangeType change);
    virtual itemcontainer& getChangingItems() { return m_changingItems; }
    virtual void endChange();
    virtual ChangeType getChangeType() { return m_changeType; }

    virtual void setLength(int width);
    virtual int  getLength();

    virtual unsigned int getHeight();
    
    void setAudioPreviewThread(AudioPreviewThread *thread);
    AudioPreviewThread* getAudioPreviewThread() { return m_audioPreviewThread; }

    void clearPreviewCache();
    void clearSegmentRectsCache(bool clearPreviews = false) { clearInCache(0, clearPreviews); }

    rectlist*         makeNotationPreviewDataCache(const Segment *s);
    AudioPreviewData* makeAudioPreviewDataCache(const Segment *s);

    CompositionRect computeSegmentRect(const Segment&, bool computeZ = false);
    QColor          computeSegmentPreviewColor(const Segment*);
    QPoint          computeSegmentOrigin(const Segment&);
    void            computeRepeatMarks(CompositionItem&);

    SegmentSelection getSelectedSegments() { return m_selectedSegments; }
    Composition&     getComposition()      { return m_composition; }
    Studio&          getStudio()           { return m_studio; }


    // CompositionObserver
    virtual void segmentAdded(const Composition *, Segment *);
    virtual void segmentRemoved(const Composition *, Segment *);
    virtual void segmentRepeatChanged(const Composition *, Segment *, bool);
    virtual void segmentStartChanged(const Composition *, Segment *, timeT);
    virtual void segmentEndMarkerChanged(const Composition *, Segment *, bool);
    virtual void segmentTrackChanged(const Composition *, Segment *, TrackId);
    virtual void endMarkerTimeChanged(const Composition *, bool /*shorten*/);

    // SegmentObserver
    virtual void eventAdded(const Segment *, Event *);
    virtual void eventRemoved(const Segment *, Event *);
    virtual void AllEventsChanged(const Segment *s);
    virtual void appearanceChanged(const Segment *);
    virtual void endMarkerTimeChanged(const Segment *, bool /*shorten*/);
    virtual void segmentDeleted(const Segment*) { /* nothing to do - handled by CompositionObserver::segmentRemoved() */ };

signals:
    void needContentUpdate();
    void needContentUpdate(const QRect&);
    void needArtifactsUpdate();
    void selectedSegments(const SegmentSelection &);
    void needSizeUpdate();

public slots:
    void slotAudioFileFinalized(Segment*);
    void slotInstrumentParametersChanged(InstrumentId);

protected slots:
    void slotAudioPreviewComplete(AudioPreviewUpdater*);

protected:
    bool setTrackHeights(Segment *changed = 0); // true if something changed

    bool isTmpSelected(const Segment*) const;
    bool wasTmpSelected(const Segment*) const;
    bool isMoving(const Segment*) const;
    bool isRecording(const Segment*) const;

    void computeRepeatMarks(CompositionRect& sr, const Segment* s);
    unsigned int computeZForSegment(const Segment* s);
        
    // segment preview stuff

    void updatePreviewCacheForNotationSegment(const Segment* s, rectlist*);
    void updatePreviewCacheForAudioSegment(const Segment* s, AudioPreviewData*);
    rectlist* getNotationPreviewData(const Segment* s);
    AudioPreviewData* getAudioPreviewData(const Segment* s);
    PixmapArray getAudioPreviewPixmap(const Segment* s);
    QRect postProcessAudioPreview(AudioPreviewData*, const Segment*);

    void makePreviewCache(const Segment* s);
    void removePreviewCache(const Segment* s);
    void makeNotationPreviewRects(RectRanges* npData, QPoint basePoint, const Segment*, const QRect&);
    void makeNotationPreviewRectsMovingSegment(RectRanges* npData, QPoint basePoint, const Segment*,
                                               const QRect&);
    void makeAudioPreviewRects(AudioPreviewDrawData* apRects, const Segment*,
                               const CompositionRect& segRect, const QRect& clipRect);

    void clearInCache(const Segment*, bool clearPreviewCache = false);
    void putInCache(const Segment*, const CompositionRect&);
    const CompositionRect& getFromCache(const Segment*, timeT& endTime);
    bool isCachedRectCurrent(const Segment& s, const CompositionRect& r,
                             QPoint segmentOrigin, timeT segmentEndTime);

    //--------------- Data members ---------------------------------
    Composition&     m_composition;
    Studio&          m_studio;
    SnapGrid         m_grid;
    SegmentSelection m_selectedSegments;
    SegmentSelection m_tmpSelectedSegments;
    SegmentSelection m_previousTmpSelectedSegments;

    timeT            m_pointerTimePos;

    typedef std::set<Segment *> recordingsegmentset;
    recordingsegmentset          m_recordingSegments;

    AudioPreviewThread*          m_audioPreviewThread;

    typedef std::map<const Segment *, rectlist *> NotationPreviewDataCache;
    typedef std::map<const Segment *, AudioPreviewData *> AudioPreviewDataCache;

    NotationPreviewDataCache     m_notationPreviewDataCache;
    AudioPreviewDataCache        m_audioPreviewDataCache;

    rectcontainer m_res;
    itemcontainer m_changingItems;
    ChangeType    m_changeType;

    typedef std::vector<CompositionItem> itemgc;
    itemgc m_itemGC;

    QRect m_selectionRect;
    QRect m_previousSelectionUpdateRect;

    std::map<const Segment*, CompositionRect> m_segmentRectMap;
    std::map<const Segment*, timeT> m_segmentEndTimeMap;
    std::map<const Segment*, PixmapArray> m_audioSegmentPreviewMap;
    std::map<TrackId, int> m_trackHeights;

    typedef std::map<const Segment*, AudioPreviewUpdater *>
        AudioPreviewUpdaterMap;
    AudioPreviewUpdaterMap m_audioPreviewUpdaterMap;

    SegmentOrderer m_segmentOrderer;

    CompositionItem* m_currentCompositionItem;

};



}

#endif
