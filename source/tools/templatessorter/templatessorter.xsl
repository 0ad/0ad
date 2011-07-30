<xsl:stylesheet
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  version="1.0">

  <xsl:output encoding="utf-8" indent="yes"/>

  <xsl:template match="@* | node() | comment()">
    <xsl:copy>
      <xsl:apply-templates select="@* | node() | comment()"/>
    </xsl:copy>
  </xsl:template>

  <!-- we use match="/Entity" because we want to sort only second-level elements (contained in top level Entity) -->
  <xsl:template match="/Entity">
    <xsl:copy>
      <xsl:apply-templates select="@* | comment()"/>
      <xsl:apply-templates select="*">
        <!-- with translate function sorting will be case-insensitive,
             because it will change lower-case letter to upper-case for sorting -->
        <xsl:sort select="translate(local-name(), 'abcdefghijklmnopqrstuvwxyz', 'ABCDEFGHIJKLMNOPQRSTUVWXYZ')"/>
      </xsl:apply-templates>
    </xsl:copy>
  </xsl:template>

</xsl:stylesheet>

