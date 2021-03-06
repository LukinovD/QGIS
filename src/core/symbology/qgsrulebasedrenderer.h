/***************************************************************************
    qgsrulebasedrenderer.h - Rule-based renderer (symbology)
    ---------------------
    begin                : May 2010
    copyright            : (C) 2010 by Martin Dobias
    email                : wonder dot sk at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSRULEBASEDRENDERERV2_H
#define QGSRULEBASEDRENDERERV2_H

#include "qgis_core.h"
#include "qgis_sip.h"
#include "qgsfields.h"
#include "qgsfeature.h"
#include "qgis.h"

#include "qgsrenderer.h"

class QgsExpression;

class QgsCategorizedSymbolRenderer;
class QgsGraduatedSymbolRenderer;

/**
 * \ingroup core
When drawing a vector layer with rule-based renderer, it goes through
the rules and draws features with symbols from rules that match.
 */
class CORE_EXPORT QgsRuleBasedRenderer : public QgsFeatureRenderer
{
  public:
    // TODO: use QVarLengthArray instead of QList

    enum FeatureFlags
    {
      FeatIsSelected = 1,
      FeatDrawMarkers = 2
    };

    // feature for rendering: QgsFeature and some flags
    struct FeatureToRender
    {
      FeatureToRender( QgsFeature &_f, int _flags )
        : feat( _f )
        , flags( _flags )
      {}
      QgsFeature feat;
      int flags; // selected and/or draw markers
    };

    // rendering job: a feature to be rendered with a particular symbol
    // (both f, symbol are _not_ owned by this class)
    struct RenderJob
    {
      RenderJob( QgsRuleBasedRenderer::FeatureToRender &_ftr, QgsSymbol *_s )
        : ftr( _ftr )
        , symbol( _s )
      {}
      QgsRuleBasedRenderer::FeatureToRender &ftr;
      QgsSymbol *symbol = nullptr;
    };

    // render level: a list of jobs to be drawn at particular level
    // (jobs are owned by this class)
    struct RenderLevel
    {
      explicit RenderLevel( int z ): zIndex( z ) {}
      ~RenderLevel() { Q_FOREACH ( RenderJob *j, jobs ) delete j; }
      int zIndex;
      QList<QgsRuleBasedRenderer::RenderJob *> jobs;

      QgsRuleBasedRenderer::RenderLevel &operator=( const QgsRuleBasedRenderer::RenderLevel &rh )
      {
        zIndex = rh.zIndex;
        qDeleteAll( jobs );
        jobs.clear();
        Q_FOREACH ( RenderJob *job, rh.jobs )
        {
          jobs << new RenderJob( *job );
        }
        return *this;
      }

      RenderLevel( const QgsRuleBasedRenderer::RenderLevel &other )
        : zIndex( other.zIndex )
      {
        Q_FOREACH ( RenderJob *job, other.jobs )
        {
          jobs << new RenderJob( *job );
        }
      }

    };

    // rendering queue: a list of rendering levels
    typedef QList<QgsRuleBasedRenderer::RenderLevel> RenderQueue;

    class Rule;
    typedef QList<QgsRuleBasedRenderer::Rule *> RuleList;

    /**
     * \ingroup core
      This class keeps data about a rules for rule-based renderer.
      A rule consists of a symbol, filter expression and range of scales.
      If filter is empty, it matches all features.
      If scale range has both values zero, it matches all scales.
      If one of the min/max scale denominators is zero, there is no lower/upper bound for scales.
      A rule matches if both filter and scale range match.
     */
    class CORE_EXPORT Rule
    {
      public:
        //! The result of rendering a rule
        enum RenderResult
        {
          Filtered = 0, //!< The rule does not apply
          Inactive,     //!< The rule is inactive
          Rendered      //!< Something was rendered
        };

        //! Constructor takes ownership of the symbol
        Rule( QgsSymbol *symbol SIP_TRANSFER, int maximumScale = 0, int minimumScale = 0, const QString &filterExp = QString(),
              const QString &label = QString(), const QString &description = QString(), bool elseRule = false );
        ~Rule();

        //! Rules cannot be copied
        Rule( const Rule &rh ) = delete;
        //! Rules cannot be copied
        Rule &operator=( const Rule &rh ) = delete;

        /**
         * Dump for debug purpose
         * \param indent How many characters to indent. Will increase by two with every of the recursive calls
         * \returns A string representing this rule
         */
        QString dump( int indent = 0 ) const;

        /**
         * Return the attributes used to evaluate the expression of this rule
         * \returns A set of attribute names
         */
        QSet<QString> usedAttributes( const QgsRenderContext &context ) const;

        /**
         * Returns true if this rule or one of its chilren needs the geometry to be applied.
         */
        bool needsGeometry() const;

        //! \note available in Python bindings as symbol2
        QgsSymbolList symbols( const QgsRenderContext &context = QgsRenderContext() ) const;

        //! \since QGIS 2.6
        QgsLegendSymbolList legendSymbolItems( int currentLevel = -1 ) const;

        /**
         * Check if a given feature shall be rendered by this rule
         *
         * \param f         The feature to test
         * \param context   The context in which the rendering happens
         * \returns          True if the feature shall be rendered
         */
        bool isFilterOK( QgsFeature &f, QgsRenderContext *context = nullptr ) const;

        /**
         * Check if this rule applies for a given \a scale.
         * The \a scale value indicates the scale denominator, e.g. 1000.0 for a 1:1000 map.
         * If set to 0, it will always return true.
         *
         * \returns If the rule will be evaluated at this scale
         */
        bool isScaleOK( double scale ) const;

        QgsSymbol *symbol() { return mSymbol; }
        QString label() const { return mLabel; }
        bool dependsOnScale() const { return mMaximumScale != 0 || mMinimumScale != 0; }

        /**
         * Returns the maximum map scale (i.e. most "zoomed in" scale) at which the rule will be active.
         * The scale value indicates the scale denominator, e.g. 1000.0 for a 1:1000 map.
         * A scale of 0 indicates no maximum scale visibility.
         * \see minimumScale()
         * \see setMaximumScale()
         * \since QGIS 3.0
        */
        double maximumScale() const { return mMaximumScale; }

        /**
         * Returns the minimum map scale (i.e. most "zoomed out" scale) at which the rule will be active.
         * The scale value indicates the scale denominator, e.g. 1000.0 for a 1:1000 map.
         * A scale of 0 indicates no minimum scale visibility.
         * \see maximumScale()
         * \see setMinimumScale()
         * \since QGIS 3.0
        */
        double minimumScale() const { return mMinimumScale; }

        /**
         * A filter that will check if this rule applies
         * \returns An expression
         */
        QgsExpression *filter() const { return mFilter; }

        /**
         * A filter that will check if this rule applies
         * \returns An expression
         */
        QString filterExpression() const { return mFilterExp; }

        /**
         * A human readable description for this rule
         *
         * \returns Description
         */
        QString description() const { return mDescription; }

        /**
         * Returns if this rule is active
         *
         * \returns True if the rule is active
         */
        bool active() const { return mIsActive; }

        /**
         * Unique rule identifier (for identification of rule within renderer)
         * \since QGIS 2.6
         */
        QString ruleKey() const { return mRuleKey; }

        /**
         * Override the assigned rule key (should be used just internally by rule-based renderer)
         * \since QGIS 2.6
         */
        void setRuleKey( const QString &key ) { mRuleKey = key; }

        //! set a new symbol (or NULL). Deletes old symbol.
        void setSymbol( QgsSymbol *sym SIP_TRANSFER );
        void setLabel( const QString &label ) { mLabel = label; }

        /**
         * Sets the minimum map \a scale (i.e. most "zoomed out" scale) at which the rule will be active.
         * The \a scale value indicates the scale denominator, e.g. 1000.0 for a 1:1000 map.
         * A \a scale of 0 indicates no minimum scale visibility.
         * \see minimumScale()
         * \see setMaximumScale()
         */
        void setMinimumScale( double scale ) { mMinimumScale = scale; }

        /**
         * Sets the maximum map \a scale (i.e. most "zoomed in" scale) at which the rule will be active.
         * The \a scale value indicates the scale denominator, e.g. 1000.0 for a 1:1000 map.
         * A \a scale of 0 indicates no maximum scale visibility.
         * \see maximumScale()
         * \see setMinimumScale()
         */
        void setMaximumScale( double scale ) { mMaximumScale = scale; }

        /**
         * Set the expression used to check if a given feature shall be rendered with this rule
         *
         * \param filterExp An expression
         */
        void setFilterExpression( const QString &filterExp );

        /**
         * Set a human readable description for this rule
         *
         * \param description Description
         */
        void setDescription( const QString &description ) { mDescription = description; }

        /**
         * Sets if this rule is active
         * \param state Determines if the rule should be activated or deactivated
         */
        void setActive( bool state ) { mIsActive = state; }

        //! clone this rule, return new instance
        QgsRuleBasedRenderer::Rule *clone() const SIP_FACTORY;

        void toSld( QDomDocument &doc, QDomElement &element, QgsStringMap props ) const;

        /**
         * Create a rule from the SLD provided in element and for the specified geometry type.
         */
        static QgsRuleBasedRenderer::Rule *createFromSld( QDomElement &element, QgsWkbTypes::GeometryType geomType ) SIP_FACTORY;

        QDomElement save( QDomDocument &doc, QgsSymbolMap &symbolMap ) const;

        //! prepare the rule for rendering and its children (build active children array)
        bool startRender( QgsRenderContext &context, const QgsFields &fields, QString &filter );

        //! get all used z-levels from this rule and children
        QSet<int> collectZLevels();

        /**
         * assign normalized z-levels [0..N-1] for this rule's symbol for quick access during rendering
         * \note not available in Python bindings
         */
        void setNormZLevels( const QMap<int, int> &zLevelsToNormLevels ) SIP_SKIP;

        /**
         * Render a given feature, will recursively call subclasses and only render if the constraints apply.
         *
         * \param featToRender The feature to render
         * \param context      The rendering context
         * \param renderQueue  The rendering queue to which the feature should be added
         * \returns             The result of the rendering. In explicit if the feature is added to the queue or
         *                     the reason for not rendering the feature.
         */
        QgsRuleBasedRenderer::Rule::RenderResult renderFeature( QgsRuleBasedRenderer::FeatureToRender &featToRender, QgsRenderContext &context, QgsRuleBasedRenderer::RenderQueue &renderQueue );

        //! only tell whether a feature will be rendered without actually rendering it
        bool willRenderFeature( QgsFeature &feat, QgsRenderContext *context = nullptr );

        //! tell which symbols will be used to render the feature
        QgsSymbolList symbolsForFeature( QgsFeature &feat, QgsRenderContext *context = nullptr );

        /**
         * Returns which legend keys match the feature
         * \since QGIS 2.14
         */
        QSet< QString > legendKeysForFeature( QgsFeature &feat, QgsRenderContext *context = nullptr );

        //! tell which rules will be used to render the feature
        QgsRuleBasedRenderer::RuleList rulesForFeature( QgsFeature &feat, QgsRenderContext *context = nullptr );

        /**
         * Stop a rendering process. Used to clean up the internal state of this rule
         *
         * \param context The rendering context
         */
        void stopRender( QgsRenderContext &context );

        /**
         * Create a rule from an XML definition
         *
         * \param ruleElem  The XML rule element
         * \param symbolMap Symbol map
         *
         * \returns A new rule
         */
        static QgsRuleBasedRenderer::Rule *create( QDomElement &ruleElem, QgsSymbolMap &symbolMap ) SIP_FACTORY;

        /**
         * Return all children rules of this rule
         *
         * \returns A list of rules
         */
        QgsRuleBasedRenderer::RuleList &children() { return mChildren; }

        /**
         * Returns all children, grand-children, grand-grand-children, grand-gra... you get it
         *
         * \returns A list of descendant rules
         */
        QgsRuleBasedRenderer::RuleList descendants() const { RuleList l; Q_FOREACH ( QgsRuleBasedRenderer::Rule *c, mChildren ) { l += c; l += c->descendants(); } return l; }

        /**
         * The parent rule
         *
         * \returns Parent rule
         */
        QgsRuleBasedRenderer::Rule *parent() { return mParent; }

        //! add child rule, take ownership, sets this as parent
        void appendChild( QgsRuleBasedRenderer::Rule *rule SIP_TRANSFER );

        //! add child rule, take ownership, sets this as parent
        void insertChild( int i, QgsRuleBasedRenderer::Rule *rule SIP_TRANSFER );

        //! delete child rule
        void removeChild( QgsRuleBasedRenderer::Rule *rule );

        //! delete child rule
        void removeChildAt( int i );

        //! take child rule out, set parent as null
        QgsRuleBasedRenderer::Rule *takeChild( QgsRuleBasedRenderer::Rule *rule ) SIP_TRANSFERBACK;

        //! take child rule out, set parent as null
        QgsRuleBasedRenderer::Rule *takeChildAt( int i ) SIP_TRANSFERBACK;

        /**
         * Try to find a rule given its unique key
         * \since QGIS 2.6
         */
        QgsRuleBasedRenderer::Rule *findRuleByKey( const QString &key );

        /**
         * Sets if this rule is an ELSE rule
         *
         * \param iselse If true, this rule is an ELSE rule
         */
        void setIsElse( bool iselse );

        /**
         * Check if this rule is an ELSE rule
         *
         * \returns True if this rule is an else rule
         */
        bool isElse() { return mElseRule; }

      protected:
        void initFilter();

        Rule *mParent; // parent rule (NULL only for root rule)
        QgsSymbol *mSymbol = nullptr;
        double mMaximumScale = 0;
        double mMinimumScale = 0;
        QString mFilterExp, mLabel, mDescription;
        bool mElseRule;
        RuleList mChildren;
        RuleList mElseRules;
        bool mIsActive; // whether it is enabled or not

        QString mRuleKey; // string used for unique identification of rule within renderer

        // temporary
        QgsExpression *mFilter = nullptr;
        // temporary while rendering
        QSet<int> mSymbolNormZLevels;
        RuleList mActiveChildren;

      private:
#ifdef SIP_RUN
        Rule( const QgsRuleBasedRenderer::Rule &rh );
#endif

        /**
         * Check which child rules are else rules and update the internal list of else rules
         *
         */
        void updateElseRules();
    };

    /////

    //! Creates a new rule-based renderer instance from XML
    static QgsFeatureRenderer *create( QDomElement &element, const QgsReadWriteContext &context ) SIP_FACTORY;

    //! Constructs the renderer from given tree of rules (takes ownership)
    QgsRuleBasedRenderer( QgsRuleBasedRenderer::Rule *root SIP_TRANSFER );
    //! Constructor for convenience. Creates a root rule and adds a default rule with symbol (takes ownership)
    QgsRuleBasedRenderer( QgsSymbol *defaultSymbol SIP_TRANSFER );

    ~QgsRuleBasedRenderer();

    //! return symbol for current feature. Should not be used individually: there could be more symbols for a feature
    virtual QgsSymbol *symbolForFeature( QgsFeature &feature, QgsRenderContext &context ) override;

    virtual bool renderFeature( QgsFeature &feature, QgsRenderContext &context, int layer = -1, bool selected = false, bool drawVertexMarker = false ) override;

    virtual void startRender( QgsRenderContext &context, const QgsFields &fields ) override;

    virtual void stopRender( QgsRenderContext &context ) override;

    virtual QString filter( const QgsFields &fields = QgsFields() ) override;

    virtual QSet<QString> usedAttributes( const QgsRenderContext &context ) const override;

    virtual bool filterNeedsGeometry() const override;

    virtual QgsRuleBasedRenderer *clone() const override SIP_FACTORY;

    virtual void toSld( QDomDocument &doc, QDomElement &element, const QgsStringMap &props = QgsStringMap() ) const override;

    static QgsFeatureRenderer *createFromSld( QDomElement &element, QgsWkbTypes::GeometryType geomType ) SIP_FACTORY;

    virtual QgsSymbolList symbols( QgsRenderContext &context ) override;

    virtual QDomElement save( QDomDocument &doc, const QgsReadWriteContext &context ) override;
    virtual bool legendSymbolItemsCheckable() const override;
    virtual bool legendSymbolItemChecked( const QString &key ) override;
    virtual void checkLegendSymbolItem( const QString &key, bool state = true ) override;

    virtual void setLegendSymbolItem( const QString &key, QgsSymbol *symbol SIP_TRANSFER ) override;
    virtual QgsLegendSymbolList legendSymbolItems() const override;
    virtual QString dump() const override;
    virtual bool willRenderFeature( QgsFeature &feat, QgsRenderContext &context ) override;
    virtual QgsSymbolList symbolsForFeature( QgsFeature &feat, QgsRenderContext &context ) override;
    virtual QgsSymbolList originalSymbolsForFeature( QgsFeature &feat, QgsRenderContext &context ) override;
    virtual QSet<QString> legendKeysForFeature( QgsFeature &feature, QgsRenderContext &context ) override;
    virtual QgsFeatureRenderer::Capabilities capabilities() override { return MoreSymbolsPerFeature | Filter | ScaleDependent; }

    /////

    QgsRuleBasedRenderer::Rule *rootRule() { return mRootRule; }

    //////

    //! take a rule and create a list of new rules based on the categories from categorized symbol renderer
    static void refineRuleCategories( QgsRuleBasedRenderer::Rule *initialRule, QgsCategorizedSymbolRenderer *r );
    //! take a rule and create a list of new rules based on the ranges from graduated symbol renderer
    static void refineRuleRanges( QgsRuleBasedRenderer::Rule *initialRule, QgsGraduatedSymbolRenderer *r );
    //! take a rule and create a list of new rules with intervals of scales given by the passed scale denominators
    static void refineRuleScales( QgsRuleBasedRenderer::Rule *initialRule, QList<int> scales );

    /**
     * creates a QgsRuleBasedRenderer from an existing renderer.
     * \since QGIS 2.5
     * \returns a new renderer if the conversion was possible, otherwise 0.
     */
    static QgsRuleBasedRenderer *convertFromRenderer( const QgsFeatureRenderer *renderer ) SIP_FACTORY;

    //! helper function to convert the size scale and rotation fields present in some other renderers to data defined symbology
    static void convertToDataDefinedSymbology( QgsSymbol *symbol, const QString &sizeScaleField, const QString &rotationField = QString() );

  protected:
    //! the root node with hierarchical list of rules
    Rule *mRootRule = nullptr;

    // temporary
    RenderQueue mRenderQueue;
    QList<FeatureToRender> mCurrentFeatures;

    QString mFilter;

  private:
#ifdef SIP_RUN
    QgsRuleBasedRenderer( const QgsRuleBasedRenderer & );
    QgsRuleBasedRenderer &operator=( const QgsRuleBasedRenderer & );
#endif
};

#endif // QGSRULEBASEDRENDERERV2_H
