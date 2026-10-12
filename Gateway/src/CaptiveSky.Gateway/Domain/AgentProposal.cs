namespace CaptiveSky.Gateway.Domain;

/// <summary>A resident-authored request that still requires human review.</summary>
public sealed record AgentProposal(
    string RequestType,
    string Description,
    string Target = "",
    string UpgradeKind = "");
