"""Migration map only. Runtime gameplay and editor tuning use grouped properties."""
GROUPS = {
    'crossfire': ('phase_duration', 'warning_duration', 'shutter_move_duration', 'team_distance', 'team_spread', 'shutter_distance', 'shutter_size'),
    'field_layout': ('field_centers', 'field_half_extent'),
    'acceleration': ('field_speed_multiplier', 'baseline_hostile_speed_multiplier', 'acceleration_sound', 'acceleration_sound_volume'),
    'refraction': ('refraction_angle_degrees',),
    'support': ('support_enemy_class', 'support_radius', 'support_attack_rate', 'support_grants_invulnerability', 'support_shield_color'),
    'zones': ('phase_duration', 'warning_duration', 'arena_half_extent', 'zone_half_extent', 'zone_center_distance', 'zone_damage', 'zone_damage_interval'),
    'mirror': ('diagonal_mirror_half_length', 'diagonal_mirror_center', 'mirror_camera_arm_length', 'lane_half_width', 'lane_half_length', 'lane_barrier_visual_height', 'shot_barrier_visual_height', 'max_mirror_bounces'),
}
