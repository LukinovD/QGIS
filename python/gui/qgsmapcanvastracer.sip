/************************************************************************
 * This file has been generated automatically from                      *
 *                                                                      *
 * src/gui/qgsmapcanvastracer.h                                         *
 *                                                                      *
 * Do not edit manually ! Edit header and run scripts/sipify.pl again   *
 ************************************************************************/



class QgsMapCanvasTracer : QgsTracer
{
%Docstring
 Extension of QgsTracer that provides extra functionality:
  - automatic updates of own configuration based on canvas settings
  - reporting of issues to the user via message bar
  - determines whether tracing is currently enabled by the user

 A simple registry of tracer instances associated to map canvas instances
 is kept for convenience. (Map tools do not need to create their local
 tracer instances and map canvas API is not "polluted" by this optional
 functionality).

.. versionadded:: 2.14
%End

%TypeHeaderCode
#include "qgsmapcanvastracer.h"
%End
  public:
    explicit QgsMapCanvasTracer( QgsMapCanvas *canvas, QgsMessageBar *messageBar = 0 );
%Docstring
Create tracer associated with a particular map canvas, optionally message bar for reporting
%End
    ~QgsMapCanvasTracer();

    QAction *actionEnableTracing() const;
%Docstring
Access to action that user may use to toggle tracing on/off. May be null if no action was associated
 :rtype: QAction
%End

    void setActionEnableTracing( QAction *action );
%Docstring
 Assign "enable tracing" checkable action to the tracer.
 The action is used to determine whether tracing is currently enabled by the user
%End

    static QgsMapCanvasTracer *tracerForCanvas( QgsMapCanvas *canvas );
%Docstring
 Retrieve instance of this class associated with given canvas (if any).
 The class keeps a simple registry of tracers associated with map canvas
 instances for easier access to the common tracer by various map tools
 :rtype: QgsMapCanvasTracer
%End

    void reportError( PathError err, bool addingVertex );
%Docstring
Report a path finding error to the user
%End

  protected:
    virtual void configure();

%Docstring
Sets configuration from current snapping settings and canvas settings
%End

};

/************************************************************************
 * This file has been generated automatically from                      *
 *                                                                      *
 * src/gui/qgsmapcanvastracer.h                                         *
 *                                                                      *
 * Do not edit manually ! Edit header and run scripts/sipify.pl again   *
 ************************************************************************/
