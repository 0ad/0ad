<?xml version="1.0"?>
<xsl:stylesheet
  xmlns="http://www.w3.org/1999/xhtml"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:rng="http://relaxng.org/ns/structure/1.0"
  xmlns:a="http://ns.wildfiregames.com/entity"
  exclude-result-prefixes="rng a"
  version="1.0">

<xsl:output method="xml" encoding="utf-8" omit-xml-declaration="yes" indent="yes"/>

<xsl:key name="defname" match="rng:define" use="@name"/>

<xsl:template match="/">
  <xsl:text disable-output-escaping="yes">&lt;!DOCTYPE html&gt;&#10;</xsl:text>
  <html>
    <head>
      <meta charset="utf-8"/>
      <title>0 A.D. entity XML documentation</title>
      <link rel="stylesheet" href="entity-docs.css"/>
    </head>
    <body>
      <h1>Entity component documentation</h1>
      <p>
      In <a href="http://play0ad.com/">0 A.D.</a>,
      entities (units and buildings and other world objects)
      consist of a collection of components, each of which determines
      part of the entity's behaviour.
      Entity template XML files specify the list of components that are loaded for
      each entity type, plus initialisation data for the components.
      </p>
      <p>
      This page lists the components that can be added to entities
      and the XML syntax for their initialisation data.
      </p>
      <p>
      Available components:
      </p>
      <xsl:apply-templates mode="components-toc"/>
      <input type="checkbox" id="show-grammar"/> <i>Display RELAX NG grammar fragments</i>
      <xsl:apply-templates mode="components"/>
    </body>
  </html>
</xsl:template>

<!-- List all the interfaces in alphabetic order -->
<xsl:template match="rng:grammar" mode="components-toc">
  <ul>
  <xsl:apply-templates select="rng:define[starts-with(@name, 'interface.')]" mode="components-toc">
    <xsl:sort select="@name"/>
  </xsl:apply-templates>
  </ul>
</xsl:template>

<xsl:template match="rng:define" mode="components-toc">
  <xsl:choose>
    <xsl:when test="count(rng:choice/rng:ref[not(key('defname', @name)//a:component/@type)]) &lt;= 1">
      <xsl:apply-templates mode="components-toc"/>
    </xsl:when>
    <xsl:otherwise> <!-- multiple implementations of the same interface: -->
      <li>
        <xsl:value-of select="substring-after(@name, 'interface.')"/>
        <ul>
          <xsl:apply-templates mode="components-toc"/>
        </ul>
      </li>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!-- List components that are not special types -->
<xsl:template match="rng:ref[not(key('defname', @name)//a:component/@type)]" mode="components-toc">
  <li>
    <a>
      <xsl:attribute name="href">
        <xsl:text>#component.</xsl:text>
        <xsl:value-of select="substring-after(@name, 'component.')"/>
      </xsl:attribute>
      <xsl:value-of select="substring-after(@name, 'component.')"/>
    </a>
    <xsl:value-of select="key('defname', @name)//a:component/@type"/>
  </li>
</xsl:template>

<!-- List all the components in alphabetic order, excluding ones that are special types -->
<xsl:template match="rng:grammar" mode="components">
  <xsl:apply-templates select="rng:define[starts-with(@name, 'component.') and not(key('defname', @name)//a:component/@type)]" mode="components">
    <xsl:sort select="@name"/>
  </xsl:apply-templates>
</xsl:template>

<!-- Component definition -->
<xsl:template match="rng:define" mode="components">
  <xsl:variable name="name" select="substring-after(@name, 'component.')"/>
  <section>
    <xsl:attribute name="id"><xsl:value-of select="@name"/></xsl:attribute>

    <h2><xsl:value-of select="$name"/></h2>

    <xsl:if test=".//a:help">
      <p><xsl:value-of select=".//a:help"/></p>
    </xsl:if>

    <xsl:if test="count(.//a:example) = 1"><h3>Example</h3></xsl:if>
    <xsl:if test="count(.//a:example) > 1"><h3>Examples</h3></xsl:if>
    <xsl:for-each select=".//a:example">
      <xsl:call-template name="example">
        <xsl:with-param name="name" select="$name"/>
      </xsl:call-template>
    </xsl:for-each>

    <xsl:apply-templates mode="components"/>

    <div class="grammar-box">
      <h3>RELAX NG Grammar</h3>
      <pre class="grammar">
        <xsl:apply-templates mode="literal"/>
      </pre>
    </div>
  </section>
</xsl:template>

<!-- Component XML example -->
<xsl:template match="*" name="example">
  <xsl:param name="name"/>
  <pre class="example">
    <xsl:choose>
      <xsl:when test="count(*) = 0">
        <xsl:text>&lt;</xsl:text>
        <span class="n"><xsl:value-of select="$name"/></span>
        <xsl:text>/&gt;&#10;</xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:text>&lt;</xsl:text>
        <span class="n"><xsl:value-of select="$name"/></span>
        <xsl:text>&gt;&#10;</xsl:text>
        <xsl:apply-templates select="*" mode="literal">
          <xsl:with-param name="depth" select="1"/>
        </xsl:apply-templates>
        <xsl:text>&lt;/</xsl:text>
        <span class="n"><xsl:value-of select="$name"/></span>
        <xsl:text>&gt;&#10;</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
  </pre>
</xsl:template>

<!-- List each of the component's elements -->
<xsl:template match="rng:element" mode="components">
  <xsl:apply-templates mode="component"/>
</xsl:template>

<!-- Component element description -->
<xsl:template match="rng:element|rng:zeroOrMore|rng:oneOrMore|rng:anyName" mode="component">
  <xsl:if test="@a:help and count(.//a:example) = 0">
    <xsl:choose>
      <xsl:when test="normalize-space(@name)=''">
        <xsl:variable name="path">
          <xsl:call-template name="getPath">
            <xsl:with-param name="node" select="."/>
          </xsl:call-template>
        </xsl:variable>
        <h4><xsl:value-of select="$path"/></h4>
      </xsl:when>
      <xsl:otherwise>
        <h3><code><xsl:value-of select="@name"/></code></h3>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:if test="parent::rng:optional">
      <p><em>Optional.</em></p>
    </xsl:if>
    <xsl:choose>
      <xsl:when test="substring(@a:help, string-length(@a:help))='.'">
        <p><xsl:value-of select="@a:help"/></p>
      </xsl:when>
      <xsl:otherwise>
        <p><xsl:value-of select="@a:help"/>.</p>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:if>
  <xsl:apply-templates mode="datatype"/>
  <xsl:apply-templates mode="component"/>
</xsl:template>

<xsl:template match="rng:attribute" mode="component">
  <h3><code><xsl:value-of select="@name"/></code></h3>
  <xsl:if test="@a:help">
    <p><xsl:value-of select="@a:help"/>.</p>
  </xsl:if>
  <xsl:apply-templates mode="datatype"/>
</xsl:template>

<xsl:template match="text()" mode="component"/> <!-- ignore text inside the grammar -->

<!-- Datatype description when all children are <value> -->
<xsl:template match="rng:choice[count(./*[not(local-name() = 'value')]) = 0]" mode="datatype">
  <p><em>Value is one of:</em></p>
  <dl>
    <xsl:apply-templates mode="choice-values"/>
  </dl>
</xsl:template>
<xsl:template match="rng:value" mode="choice-values">
  <dt><code><xsl:value-of select="text()"/></code></dt>
  <xsl:if test="@a:help">
    <dd><xsl:value-of select="@a:help"/>.</dd>
  </xsl:if>
</xsl:template>

<!-- Datatype description for fixed string -->
<xsl:template match="rng:value" mode="datatype">
  <p><em>Required value:</em>
  <xsl:text> </xsl:text>
  <code><xsl:value-of select="text()"/></code></p>
</xsl:template>

<!-- Datatype description for text -->
<xsl:template match="rng:text" mode="datatype">
  <p><em>Value type:</em> text.</p>
</xsl:template>

<!-- Datatype description for <data> -->
<xsl:template match="rng:data[@type]" mode="datatype">
  <p><em>Value type:</em>
  <xsl:text> </xsl:text>
  <xsl:apply-templates select="@type" mode="datatype-name"/>.</p>
</xsl:template>

<!-- Datatype description for <ref> -->
<xsl:template match="rng:ref" mode="datatype">
  <p><em>Value type:</em>
  <xsl:text> </xsl:text>
  <xsl:apply-templates select="@name" mode="datatype-name"/>.</p>
</xsl:template>

<xsl:template match="*" mode="datatype"/> <!-- don't recurse -->

<!-- Datatype names -->
<xsl:template match="@*" mode="datatype-name">
  <xsl:choose>
    <xsl:when test=". = 'nonNegativeDecimal'">non-negative decimal (e.g. <code>0.0</code> or <code>2.5</code>)</xsl:when>
    <xsl:when test=". = 'positiveDecimal'">positive decimal (e.g. <code>1.0</code> or <code>2.5</code>)</xsl:when>
    <xsl:when test=". = 'decimal'">decimal (e.g. <code>-10.0</code> or <code>0.0</code> or <code>2.5</code>)</xsl:when>
    <xsl:when test=". = 'nonNegativeInteger'">non-negative integer (e.g. <code>0</code> or <code>5</code>)</xsl:when>
    <xsl:when test=". = 'positiveInteger'">positive integer (e.g. <code>1</code> or <code>5</code>)</xsl:when>
    <xsl:when test=". = 'boolean'">boolean (<code>true</code> or <code>false</code>)</xsl:when>
    <xsl:otherwise><xsl:value-of select="."/></xsl:otherwise>
  </xsl:choose>
</xsl:template>





<!-- Templates to output the input grammar as a pretty-printed string: -->

<xsl:template match="node()" mode="literal">
  <xsl:param name="depth">0</xsl:param>

  <xsl:call-template name="indent"><xsl:with-param name="depth" select="$depth"/></xsl:call-template>
  <xsl:text>&lt;</xsl:text>
  <span class="n"><xsl:value-of select="name()"/></span>
  <xsl:apply-templates select="@*" mode="literal"/>

  <xsl:if test="count(*|text()) = 0">
    <xsl:text>/&gt;&#10;</xsl:text>
  </xsl:if>
  <xsl:if test="count(*|text()) > 0">
    <xsl:text>&gt;</xsl:text>
    <xsl:if test="count(*) > 0">
      <xsl:text>&#10;</xsl:text>
    </xsl:if>

    <xsl:apply-templates select="node()" mode="literal">
      <xsl:with-param name="depth" select="$depth + 1"/>
    </xsl:apply-templates>

    <xsl:if test="count(*) > 0">
      <xsl:call-template name="indent"><xsl:with-param name="depth" select="$depth"/></xsl:call-template>
    </xsl:if>

    <xsl:text>&lt;/</xsl:text>
    <span class="n"><xsl:value-of select="name()"/></span>
    <xsl:text>&gt;&#10;</xsl:text>
  </xsl:if>
</xsl:template>

<xsl:template match="@*" mode="literal">
  <xsl:text> </xsl:text>
  <span class="n"><xsl:value-of select="name()"/></span>
  <xsl:text>="</xsl:text>
  <xsl:value-of select="."/>
  <xsl:text>"</xsl:text>
</xsl:template>

<xsl:template match="a:*|@a:*" mode="literal"/>

<xsl:template match="text()" mode="literal">
  <xsl:choose>
    <xsl:when test="normalize-space()=''">
    </xsl:when>
    <xsl:otherwise>
      <xsl:value-of select="."/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="indent">
  <xsl:param name="depth"/>
  <xsl:if test="$depth > 0">
    <xsl:text>  </xsl:text>
    <xsl:call-template name="indent">
      <xsl:with-param name="depth" select="$depth - 1"/>
    </xsl:call-template>
  </xsl:if>
</xsl:template>

<xsl:template name="getPath">
  <xsl:param name="node" />
  <xsl:variable name="maxDepth" select="1"/>
  <xsl:choose>
    <xsl:when test="not($node/..)">
      <xsl:if test="$node/ancestor::*[position() > $maxDepth]">
        <xsl:choose>
          <xsl:when test="$node/@name">
            <xsl:value-of select="concat($node/@name)" />
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="name($node)" />
          </xsl:otherwise>
        </xsl:choose>
      </xsl:if>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="getPath">
        <xsl:with-param name="node" select="$node/.." />
      </xsl:call-template>
      <xsl:if test="$node/ancestor::*[position() > $maxDepth]">
        <xsl:if test="$node/ancestor::*[position() > ($maxDepth + 1)]">
          <xsl:text>.</xsl:text>
        </xsl:if>
        <xsl:choose>
          <xsl:when test="$node/@name">
            <xsl:value-of select="$node/@name" />
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="name($node)" />
          </xsl:otherwise>
        </xsl:choose>
      </xsl:if>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>
</xsl:stylesheet>

